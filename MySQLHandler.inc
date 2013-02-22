<script language="php">
/**
 * @package mysql_handler
 * @copyright Copyright © 2001-2013 Ten&Two Systems. All Rights Reserved.
 */

/**
 * @see MySQLHandlerConfig
 */
require_once("mysql_handler/config.inc");

/**
 * Generic interface to MySQL
 *
 * Handle basic communication with MySQL, ie, run queries and return results.
 *
 * @package mysql_handler
 */
class MySQLHandler {
	/**
	 * @var object MySQLHandlerConfig object
	 */
	public $config;

	/**
	 * @var resource returned by MySQLHandler::openConnection()
	 */
	public $dbhandle;

	/**
	 * @var string Set in MySQLHandler::setDBQuery()
	 */
	public $dbquery;

	/**
	 * @var string constant used in MySQLHandler::listMeta()
	 */
	public $meta_query;

	/**
	 * @var string constant used in MySQLView::fixQuotes()
	 */
	public $meta_column_type;

	/**
	 * @var string constant used in MySQLHandler::listMeta()
	 */
	public $meta_column_name;

	/**
	 * @var string constant used in MySQLView::checkRequiredInsertFields() and MySQLView::checkRequiredUpdateFields()
	 */
	public $meta_nullable;

	/**
	 * @var string constant used in MySQLView::checkRequiredInsertFields() and MySQLView::checkRequiredUpdateFields()
	 */
	public $meta_nullable_true;

	/**
	 * @param array $params override config values
	 * - string dbhost
	 * - string dbname
	 * - string dbuser
	 * - bool close_connections
	 * - bool load_data_local
	 * - string export_path
	 */
	public function MySQLHandler( $params = array() ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)<br/>\n";
		$this->config = new MySQLHandlerConfig();

		$this->dbhandle = null;
		$this->dbquery = "";

		while (list($key,$val) = each($params)) {
			// override member values
			if ($key == "dbquery") { $this->setDBQuery($val); }
			// config values
			else if ($key == "dbhost") { $this->config->dbhost = $val; }
			else if ($key == "dbname") { $this->config->dbname = $val; }
			else if ($key == "dbuser") { $this->config->dbuser = $val; }
			else if ($key == "dbpass") { $this->config->dbpass = $val; }
			else if ($key == "close_connections") { $this->config->close_connections = $val; }
			else if ($key == "load_data_local")   { $this->config->load_data_local = $val; }
			else if ($key == "export_path")       { $this->config->export_path = $val; }
		}

		// CONSTANTS
		$this->meta_query = "SHOW COLUMNS FROM";  // constant, see listMeta
		$this->meta_column_type = "Type";         // constant, result keyname
		$this->meta_column_name = "Field";        // constant, result keyname
		$this->meta_nullable = "Null";            // constant, result keyname
		$this->meta_nullable_true = "YES";        // constant, result value
		$this->meta_default = "Default";          // constant, result keyname

		$this->databases_query = "SHOW DATABASES";  // constant, see listDatabases
		$this->databases_result = "Database";       // constant, see listDatabases

		$this->tables_query = "SHOW TABLES FROM ";  // constant, see listTables
		$this->tables_result = "Tables_in_";        // constant, see listTables

//		$this->openConnection();
	}

	/**
	 * @uses MySQLHandler::closeConnection()
	 */
	public function __destruct() {
//echo "<b>".__METHOD__."</b><br/>\n";
		$this->closeConnection();
	}
//	public function __clone() { echo "<b>".__METHOD__."</b><br/>\n"; }

	/**
	 * Set database connection according to config values.
	 *
	 * @param array $params override connection values
	 * - string dbhost
	 * - string dbname
	 * - string dbuser
	 * - string dbpass
	 * @return bool           true for success ; false for error
	 */
	public function openConnection( $params = array() ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)<br/>\n";

		if (!empty($params['dbhost'])) { $this->config->dbhost = $params['dbhost']; }
		if (!empty($params['dbname'])) { $this->config->dbname = $params['dbname']; }
		if (!empty($params['dbuser'])) { $this->config->dbuser = $params['dbuser']; }
		if (!empty($params['dbpass'])) { $this->config->dbpass = $params['dbpass']; }

		$this->dbhandle = @mysql_connect($this->config->dbhost,$this->config->dbuser,$this->config->dbpass);
		if (!is_resource($this->dbhandle))
			{ die("ERROR: Could not connect to DB: " . mysql_errno() . " : " . mysql_error()); }
		if (!mysql_select_db($this->config->dbname,$this->dbhandle))
			{ die("ERROR: Could not select DB: " . mysql_errno() . " : " . mysql_error()); }

		return true;
	}

	/**
	 * Call mysql_close() and unset connection.
	 * @return bool           true for success ; false for error
	 */
	public function closeConnection() {
//echo "<b>".__METHOD__."</b>()<br/>\n";
		if (is_resource($this->dbhandle) && !mysql_close($this->dbhandle))
			{ die(__METHOD__." ERROR: mysql_close failed. "); }
		$this->dbhandle = null;
		return true;
	}

	/**
	 * Override connection.
	 * @param resource $dbhandle
	 */
	public function setDBHandle( $dbhandle ) {
//echo "<b>".__METHOD__."</b>()<br/>\n";
		if (is_resource($dbhandle)) { $this->dbhandle = $dbhandle; }
		else { die(__METHOD__." ERROR: invalid param dbhandle=".gettype($dbhandle)); }
	}

	/**
	 * Set query string prior to runQuery();
	 * @param string $dbquery
	 */
	public function setDBQuery( $dbquery ) {
//echo "<b>".__METHOD__."</b>(".$dbquery.")<br/>\n";
		if (!empty($dbquery)) { $this->dbquery = $dbquery; }
		else { die(__METHOD__." ERROR: invalid param dbquery=$dbquery"); }
	}

	/**
	 * Set query string prior to runQuery();
	 * @param string $procedure_name : args[0]
	 * @param variable $procedure_arg : args[1..n]
	 */
	public function setDBProcedure() {
		$args = func_get_args();
//echo "<b>".__METHOD__."</b>(".print_r($args,true).")<br/>\n";

		$procedure_name = func_get_arg(0);
		if (empty($procedure_name))
			{ die(__METHOD__." ERROR: missing arg[0] 'procedure_name' "); }

		$dbquery = "CALL " . $procedure_name . "(";
		for ($i = 1; $i < count($args); $i++) {
			if ($i > 1) { $dbquery .= ", "; }
			$dbquery .= (is_numeric($args[$i])) ? $args[$i] : "'" . $args[$i] . "'";
		}
		$dbquery .= ");";

		$this->setDBQuery($dbquery);
	}

	/**
	 * Create and execute statement, then fetch rows into result array
	 *
	 * @param array &$result
	 * @param string &$err    error message, if applicable
	 * @return bool           true for success ; false for error
	 */
	public function runQuery( &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(".$this->dbquery.")<br/>\n";

		$this->openConnection();
		$result = array();
		$err = '';

		if (!is_resource($this->dbhandle))
			{ $err = __METHOD__." ERROR: invalid member resource this->dbhandle=".$this->print($this->dbhandle); return false; }
		if (empty($this->dbquery))
			{ $err = __METHOD__." ERROR: invalid member string this->dbquery=".$this->dbquery; return false; }

		$dbresult = mysql_query( $this->dbquery, $this->dbhandle );
		if (!$this->noError($dbresult,$err)) { return false; }

		if (is_resource($dbresult)) {
			while ($row = mysql_fetch_array($dbresult,MYSQL_ASSOC))
				{ $result[] = $row; }

			if (!mysql_free_result($dbresult))
				{ $err = __METHOD__." ERROR: could not free result"; return false; }

		} else if (!(strpos(strtoupper($this->dbquery),"INSERT ") === false)
				|| !(strpos(strtoupper($this->dbquery),"UPDATE ") === false)
				|| !(strpos(strtoupper($this->dbquery),"DELETE ") === false)
				|| !(strpos(strtoupper($this->dbquery),"LOAD DATA ") === false)
		) {
			$result = array(
				"affected_rows" => mysql_affected_rows($this->dbhandle)
				);
		}

		if ($this->config->close_connections)
			{ $this->closeConnection(); }

		return true;
	}

	/**
	 * Check mysql_error ; format error string
	 *
	 * @param resource &$result
	 * @param string &$err
	 * @return bool           true for success ; false for error
	 */
	protected function noError( &$result, &$err ) {
		if (mysql_errno()) {
			if (is_resource($result) && !mysql_free_result($result))
				{ $err = __METHOD__." ERROR: could not free result"; return false; }

			$err = "MySQL ERROR:"
				. "<br/>\nQUERY: " . $this->dbquery
				. "<br/>\n" . mysql_errno() . " : " . mysql_error();
			return false;
		}
		return true;
	}

	/**
	 * List available databases
	 *
	 * @param array $params
	 * @param array &$result
	 * @param string &$err
	 * @return bool           true for success ; false for error
	 */
	public function listDatabases( $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)<br/>\n";

		$result = array();
		$err = '';

		$this->setDBQuery($this->databases_query);
		if (!$this->runQuery( $data, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		while (list($key,$val) = each($data))
			{ $result[] = $val[$this->databases_result]; }

		return true;
	}

	/**
	 * List available tables within database
	 *
	 * @param array $params
	 * @param array &$result
	 * @param string &$err
	 * @return bool           true for success ; false for error
	 */
	public function listTables( $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)<br/>\n";

		$result = array();
		$err = '';

		$this->setDBQuery($this->tables_query . $this->config->dbname);
		if (!$this->runQuery( $data, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		while (list($key,$val) = each($data))
			{ $result[] = $val[$this->tables_result . $this->config->dbname]; }

		return true;
	}

	/**
	 * Fetch metadata for requested table into global array.
	 *
	 * @global array $MYSQL_METADATA array("tblname" => array("fldname" => array(Field,Type,Null,Key,Default,Extra)))
	 *
	 * @param array $params 1 required param
	 * - string table
	 * @param array &$result
	 * @param string &$err
	 * @return bool           true for success ; false for error
	 */
	public function listMeta( $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)<br/>\n";

		global $MYSQL_METADATA;
		$result = array();
		$err = '';

		if (empty($params['table']))
			{ $err = __METHOD__." ERROR: missing param 'table'"; return false; }
		if (empty($MYSQL_METADATA[$params['table']])) { $MYSQL_METADATA[$params['table']] = null; }

		// already created? just return it
		if (is_array($MYSQL_METADATA[$params['table']]))
			{ $result = $MYSQL_METADATA[$params['table']]; return true; }

		$this->setDBQuery($this->meta_query . " " . $params['table']);
		if (!$this->runQuery( $data, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// result array indexed by fieldname
		while (list($key,$val) = each($data))
			{ $result[$val[$this->meta_column_name]] = $val; }

		$MYSQL_METADATA[$params['table']] = $result;
//echo __METHOD__.": MYSQL_METADATA[".$params['table']."] = <pre>"; print_r($MYSQL_METADATA[$params['table']]); echo "</pre>\n";

		// created now?
		if (is_array($MYSQL_METADATA[$params['table']]))
			{ $result = $MYSQL_METADATA[$params['table']]; return true; }

		$err = __METHOD__." ERROR: unable to get meta data for table '" . $params['table'] . "'";
		return false;
	}

	/**
	 * Return class methods and members
	 * @return string
	 */
	public function toString() {
		ob_start();                   // start output buffering

		echo "<b>BEGIN ".__METHOD__."</b> ( \n";
		echo "\n<b>class_methods</b> ("; print_r(get_class_methods($this)); echo ")";
		echo "\n<b>class_members</b> ("; print_r($this); echo ")";
		echo "\n) <b>END ".__METHOD__."</b> \n";

		$result = ob_get_contents();  // get buffer content
		ob_end_clean();               // stop output buffering

		return $result;               // return content
	}

}

</script>
