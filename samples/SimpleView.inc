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
 * Uses single table for insert, update, delete, and select
 * @see SimpleDBHandler
 * @see sample.sql CREATE TABLE simple_item
 * @package mysql_handler
 * @subpackage samples
 */
class SimpleView extends MySQLView
{

	/**
	 * Config arrays: tables, update, select, tableinfo
	 */
	function SimpleView( $params = array() ) {

		$this->tables = array("simple_item");

		$this->update = array(
			"byItemId" => "'WHERE ixItem=' . \$params['ixItem']"
			);

		$this->select = array(
			"all" => "'ORDER BY sItem'"
			, "byItemId" => "'WHERE ixItem = ' . \$params['ixItem']"
			, "byItemName" => "'WHERE BINARY sItem = \"' . \$params['sItem'] . '\"'"
			, "byStartsWith" => "'WHERE sItem LIKE \"' . \$params['startsWith'] . '%\""
					. " ORDER BY sItem'"
			);

		$this->tableinfo = array(
			"simple_item" => array(
				"delete" => array(
					"byItemId" => "'WHERE ixItem=' . \$params['ixItem']"
					)
				, "map" => array(
					"ixItem" => "ixItem"
					, "sItem" => "sItem"
					, "mod_update" => array("column" => "mod_update"
										, "select" => "mod_update"
										, "insert" => "NOW()"
										, "update" => "NOW()"
										)
					, "mod_create" => array("column" => "mod_create"
										, "select" => "mod_create"
										, "insert" => "NOW()"
										)
					)
				)
		);

		// call superclass
		$this->MySQLView($params);
	}

}  // end class SimpleView
