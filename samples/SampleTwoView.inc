<?php
/**
 * @package mysql_handler
 * @subpackage samples
 * @copyright Copyright © 2001-2017 Ten&Two Systems. All Rights Reserved.
 */

/**
 * @see MySQLView
 */
require_once("mysql_handler/MySQLView.inc");

/**
 * Uses 2 tables, table aliasing
 * @see sample.sql CREATE TABLE parent & CREATE TABLE child
 * @package mysql_handler
 * @subpackage samples
 */
class SampleTwoView extends MySQLView
{

	/**
	 * Config arrays: tables, update, select, tableinfo
	 */
	function SampleTwoView( $params = array() ) {

		$this->tables = array(
			array("t1" => "parent")
			, array("t2" => "child")
			);
		$join = "'WHERE t1.ixParent = t2.ixParent '";

		$this->update = array(
			"byChildId" => $join . "'AND t2.ixChild = ' . \$params['ixChild']"
			);

		$this->select = array(
			"all" => $join
					. "'ORDER BY sParent, sChild'"
			, "byParentIdIn" => $join
					. "'AND t1.ixParent IN (' . \$params['ixParent'] . ')"
					. " ORDER BY sParent, sChild'"

			, "byParentId" => $join
					. "'AND t1.ixParent = ' . \$params['ixParent']"
			, "byParentName" => $join
					. "'AND t1.sParent = \"' . \$params['sParent'] . '\""
					. " ORDER BY sChild'"
			, "byStartsWith" => $join
					. "'AND t1.sParent LIKE \"' . \$params['startsWith'] . '%\""
					. " ORDER BY sParent, sChild'"

			, "byChildId" => $join
					. "'AND t2.ixChild = ' . \$params['ixChild'] . '"
					. " ORDER BY sParent'"
			, "byChildName" => $join
					. "'AND t2.sChild = \"' . \$params['sChild'] . '\""
					. " ORDER BY sParent'"
			);

		$this->tableinfo = array(
			"parent" => array(
				"delete" => array(
					"byParentId" => "'WHERE ixParent = ' . \$params['ixParent']"
					)
				, "map" => array(
					"ixParent" => "ixParent"
					, "sParent" => "sParent"
					)
				)
			, "child" => array(
				"delete" => array(
					"byParentId" => "'WHERE ixParent = ' . \$params['ixParent']"
					)
				, "map" => array(
					"ixChild" => "ixChild"
					, "sChild" => "sChild"
//					, "ixParent" => "ixParent"    // don't select value twice
					)
				)
		);

		// call superclass
		$this->MySQLView($params);
	}

}  // end class SampleTwoView
