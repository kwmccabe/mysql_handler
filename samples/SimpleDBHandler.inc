<?php
/**
 * @package mysql_handler
 * @subpackage samples
 * @copyright Copyright © 2001-2017 Ten&Two Systems. All Rights Reserved.
 */

/**
 * @see MySQLHandler
 */
require_once("mysql_handler/MySQLHandler.inc");


/**
 * Demo handler uses SimpleView
 * Uses single table for insert, update, delete, and select
 * @see SimpleView
 * @package mysql_handler
 * @subpackage samples
 */
class SimpleDBHandler extends MySQLHandler
{
	/*
	 * @var array MySQLView array
	 */
	var $views;

	/*
	 * @var string validation regular expression
	 */
	var $regexp_email;

	/*
	 * @var string validation regular expression
	 */
	var $regexp_name;

	function SimpleDBHandler( $params = array() ) {
		$this->MySQLHandler( $params );

		$this->regexp_email = "^[_\.0-9a-z-]+@([0-9a-z-]+\.)+[a-z]{2,3}$";
		$this->regexp_name  = "^[_\.0-9a-z-]{4,31}$";
	}

	/**
	 * Return MySQLView by key
	 *
	 * @param array $params 1 required param
	 * - string view
	 * @param MySQLView &$result
	 * @param string &$err    if applicable
	 * @return bool           true for success ; false for error
	 */
	function getView( $params, &$result, &$err ) {
//echo get_class($this)."::<b>getView</b>(<pre>"; print_r($params); echo "</pre>)<br/>\n";

		if (!strlen($params['view']))
			{ $err = get_class($this)."::getView ERROR: missing param 'view' "; return false; }
		if (!isset($this->views[$params['view']])) { $this->views[$params['view']] = null; }

		// already created? just return it
		if (is_object($this->views[$params['view']]))
			{ $result = $this->views[$params['view']]; return true; }

		// create view according to $params['view']
		$params['dbhandler'] = $this;
		if ($params['view'] == "simple") {
			require_once("simple/db/SimpleView.inc");
			$this->views[$params['view']] = new SimpleView( $params );
		}
		// more views here

		// created now? return it
		if (is_object($this->views[$params['view']]))
			{ $result = $this->views[$params['view']]; return true; }

		$err = get_class($this)."::getView ERROR: unknown param 'view' (" . $params['view'] . ")";
		return false;
	}

	/**
	 * Get next primary key from simple_idx.tablename
	 *
	 * @see sample.sql : CREATE TABLE simple_idx ( ... )
	 *
	 * @param array $params 1 required param
	 * - string table
	 * @param integer &$result same as DB column type
	 * @param string &$err     if applicable
	 * @return bool            true for success ; false for error
	 */
	function getPrimaryKey( $params, &$result, &$err ) {
//echo get_class($this)."::<b>getPrimaryKey</b>(<pre>"; print_r($params); echo "</pre>)<br>\n";

		$keys_tblname = "simple_idx";

		if (!$params['table'])
			{ $err = "getPrimaryKey ERROR: missing param 'table'"; return false; }

		$this->dbquery = "LOCK TABLES ".$keys_tblname." WRITE";
		if (!$this->runQuery( $lockresult, $err )) { $result = $lockresult; return false; }

		$this->dbquery = "update ".$keys_tblname." set ".$params['table']."=".$params['table']."+1;";
		if (!$this->runQuery( $result, $err )) { return false; }

		$this->dbquery = "select ".$params['table']." from ".$keys_tblname.";";
		if (!$this->runQuery( $result, $err )) { return false; }

		$result = $result[0][$params['table']];

		$this->dbquery = "UNLOCK TABLES";
		if (!$this->runQuery( $lockresult, $err )) { $result = $lockresult; return false; }

//echo get_class($this)."::getPrimaryKey: result="; print_r($result); echo "<br>\n";
		return true;
	}

	/**
	 * Delete row from simple_item using ixItem
	 *
	 * @param array $params 1 required param
	 * - integer ixItem
	 * @param array &$result   resultset
	 * @param string &$err     if applicable
	 * @return bool            true for success ; false for error
	 */
	function deleteItem( $params, &$result, &$err ) {
//echo get_class($this)."::<b>deleteItem</b>(<pre>"; print_r($params); echo "</pre>)<br>\n";

		if (!$params['ixItem'])
			{ $err = get_class($this)."::deleteItem ERROR: missing primary key: ixItem "; return false; }

		if (!$this->getView( array("view" => "simple"), $view, $terr ))
			{ $err = get_class($this)."::deleteItem ERROR: $terr "; return false; }
		if (!$view->deleteView( "byItemId", $params, $result, $terr ))
			{ $err = get_class($this)."::deleteItem ERROR: $terr "; return false; }

//echo get_class($this)."::deleteItem: result=<pre>"; print_r($result); echo "</pre><br/>\n";
		return true;
	}

	/**
	 * Insert new row into simple_item
	 *
	 * @uses SimpleDBHandler::getPrimaryKey()
	 * @uses SimpleDBHandler::getView()
	 * @uses MySQLView::insertView()
	 * @uses SimpleDBHandler::listItem()
	 *
	 * @param array $params    posted form field values
	 * @param array &$result   resultset
	 * @param string &$err     if applicable
	 * @return bool            true for success ; false for error
	 */
	function insertItem( $params, &$result, &$err ) {
//echo get_class($this)."::<b>insertItem</b>(<pre>"; print_r($params); echo "</pre>)<br>\n";

		if (!$this->getPrimaryKey(array("table" => "simple_item"),$params['ixItem'],$err))
			{ return false; }

		if (!$this->getView( array("view" => "simple"), $view, $terr ))
			{ $err = get_class($this)."::insertItem ERROR: $terr "; return false; }
		if (!$view->insertView( $params, $result, $terr ))
			{ $err = get_class($this)."::insertItem ERROR: $terr "; return false; }

		$params = array(
			"ixItem" => $params['ixItem']
			);
		if (!$this->listItem( $params, $result, $terr ))
			{ $err = get_class($this)."::insertItem ERROR: $terr "; return false; }

//echo get_class($this)."::insertItem: result=<pre>"; print_r($result); echo "</pre><br/>\n";
		return true;
	}

	/**
	 * Update existing row in simple_item
	 *
	 * @uses SimpleDBHandler::insertItem()
	 * @uses SimpleDBHandler::getView()
	 * @uses MySQLView::updateView()
	 * @uses SimpleDBHandler::listItem()
	 *
	 * @param array $params    posted form field values
	 * @param array &$result   resultset
	 * @param string &$err     if applicable
	 * @return bool            true for success ; false for error
	 */
	function updateItem( $params, &$result, &$err ) {
//echo get_class($this)."::<b>updateItem</b>(<pre>"; print_r($params); echo "</pre>)<br>\n";

		if ($params['sEmail'] = trim(strtolower($params['sEmail']))) {
			if (!eregi($this->regexp_email, $params['sEmail']))
				{ $err = "Invalid email address.  Please try again."; return false; }
		}
		if ($params['sItem'] = trim(strtolower($params['sItem']))) {
			if (!eregi($this->regexp_name, $params['sItem']))
				{ $err = "Invalid item name.  Please try again."; return false; }
		}

		// redirect to insert if primary key missing
		if (!($params['ixItem'] > 0)) {
			return $this->insertItem($params,$result,$err);
		}

		if (!$this->getView( array("view" => "simple"), $view, $terr ))
			{ $err = get_class($this)."::updateItem ERROR: $terr "; return false; }
		if (!$view->updateView( "byItemId", $params, $result, $terr ))
			{ $err = get_class($this)."::updateItem ERROR: $terr "; return false; }

		$params = array(
			"ixItem" => $params['ixItem']
			);
		if (!$this->listItem( $params, $result, $terr ))
			{ $err = get_class($this)."::updateItem ERROR: $terr "; return false; }

//echo get_class($this)."::updateItem: result=<pre>"; print_r($result); echo "</pre><br/>\n";
		return true;
	}

	/**
	 * Fetch existing row(s) from simple_item
	 *
	 * @uses SimpleDBHandler::getView()
	 * @uses MySQLView::selectView()
	 *
	 * @param array $params    posted form field values
	 * @param array &$result   resultset
	 * @param string &$err     if applicable
	 * @return bool            true for success ; false for error
	 */
	function listItem( $params, &$result, &$err ) {
//echo get_class($this)."::<b>listItem</b>(<pre>"; print_r($params); echo "</pre>)<br>\n";

		if ($params['ixItem'] == -1) {
			$result[0]['ixItem'] = -1;
			$result[0]['sItem']  = "New Item";
			return true;
		}

		if (!$this->getView( array("view" => "simple"), $view, $terr ))
			{ $err = get_class($this)."::listItem ERROR: $terr "; return false; }

		if ($params['ixItem'] > 0) {
			if (!$view->selectView("byItemId",$params,$result,$terr))
				{ $err = get_class($this)."::listItem ERROR: $terr"; return false; }
		} else if (strlen($params['sItem'])) {
			if (!$view->selectView("byItemName",$params,$result,$terr))
				{ $err = get_class($this)."::listItem ERROR: $terr"; return false; }
		} else if (strlen($params['startsWith'])) {
			if (!$view->selectView("byStartsWith",$params,$result,$terr))
				{ $err = get_class($this)."::listItem ERROR: $terr"; return false; }
		} else {
			if (!$view->selectView("all",$params,$result,$terr))
				{ $err = get_class($this)."::listItem ERROR: $terr"; return false; }
		}

		// manipulate (change/add) resultset, if necessary, here

		return true;
	}

}  // end class SimpleDBHandler
