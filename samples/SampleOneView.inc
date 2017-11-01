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
 * Uses 2 tables
 * @see sample.sql CREATE TABLE parent & CREATE TABLE child
 * @package mysql_handler
 * @subpackage samples
 */
class SampleOneView extends MySQLView
{

	/**
	 * Config arrays: tables, update, select, tableinfo
	 */
	function SampleOneView( $params = array() ) {

		$this->tables = array(
			"parent"
			, "child"
			);
		$join = "'WHERE parent.ixParent = child.ixParent '";

		$this->update = array(
			"byChildId" => $join . "'AND child.ixChild = ' . \$params['ixChild']"
			);

		$this->select = array(
			"all" => $join
					. "'ORDER BY sParent, sChild'"
			, "byParentIdIn" => $join
					. "'AND parent.ixParent IN (' . \$params['ixParent'] . ')"
					. " ORDER BY sParent, sChild'"

			, "byParentId" => $join
					. "'AND parent.ixParent = ' . \$params['ixParent']"
			, "byParentName" => $join
					. "'AND parent.sParent = \"' . \$params['sParent'] . '\""
					. " ORDER BY sChild'"
			, "byStartsWith" => $join
					. "'AND parent.sParent LIKE \"' . \$params['startsWith'] . '%\""
					. " ORDER BY sParent, sChild'"

			, "byChildId" => $join
					. "'AND child.ixChild = ' . \$params['ixChild'] . '"
					. " ORDER BY sParent'"
			, "byChildName" => $join
					. "'AND child.sChild = \"' . \$params['sChild'] . '\""
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

}  // end class SampleOneView
