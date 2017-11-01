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
 * Uses 2 tables, table aliasing, join_type and join_condition
 * @see sample.sql CREATE TABLE parent & CREATE TABLE child
 * @package mysql_handler
 * @subpackage samples
 */
class SampleThreeView extends MySQLView
{

	/**
	 * Config arrays: tables, update, select, tableinfo
	 */
	function SampleThreeView( $params = array() ) {

		$this->tables = array(
			array(
				"tblalias" => "t1"
				, "tblname" => "parent"
				)
			, array(
				"tblalias" => "t2"
				, "tblname" => "child"
				, "join_type" => "LEFT OUTER"
				, "join_condition" => "'t1.ixParent = t2.ixParent'" // evaluated
				)
			);

		$this->update = array();

		$this->select = array(
			"all" => "'ORDER BY sParent, sChild'"
			, "byParentIdIn" => "'WHERE t1.ixParent IN (' . \$params['ixParent'] . ')"
					. " ORDER BY sParent, sChild'"

			, "byParentId" => "'WHERE t1.ixParent = ' . \$params['ixParent'] . '"
					. " ORDER BY sChild'"
			, "byParentName" => "'WHERE t1.sParent = \"' . \$params['sParent'] . '\""
					. " ORDER BY sChild'"
			, "byStartsWith" => "'WHERE t1.sParent LIKE \"' . \$params['startsWith'] . '%\""
					. " ORDER BY sParent, sChild'"

			, "byChildId" => "'WHERE t2.ixChild = ' . \$params['ixChild'] . '"
					. " ORDER BY sParent'"
			, "byChildName" => "'WHERE t2.sChild = \"' . \$params['sChild'] . '\""
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

}  // end class SampleThreeView
