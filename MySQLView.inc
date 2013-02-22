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
 * Automatically generates and runs sql for configured database views.
 *
 * Subclasses implement specific database views by initializing the following member arrays.
 *
 * <b>tables</b> :
 * - read fwd for insert(), rev for delete()
 * - option one:
 * <code>
 * $this->tables = array(
 *     "table_one"
 *     , "table_two"
 *     );
 * </code>
 * - option two:
 * <code>
 * $this->tables = array(
 *     array("t1" => "table_one")
 *     , array("t2" => "table_two")
 *     );
 * </code>
 * - option three:
 * <code>
 * $this->tables = array(
 *     array(
 *         "tblname" => "table_one"
 *         , "tblalias" => "t1"
 *         )
 *     , array(
 *         "tblname" => "table_two"
 *         , "tblalias" => "t2"
 *         , "join_type" => "[INNER | LEFT]"
 *         , "join_condition" => "'t1.ixChild = t2.ixItem'"  // evaluated
 *         )
 *     );
 * </code>
 *
 * <b>update</b> :
 * - array("updatekey" => {where-clause})
 * - where-clause evaluted to create update statement
 * <code>
 * $this->update = array(
 *     "byId" => "'WHERE idx=' . \$params['idx']"
 *     );
 * </code>
 *
 * <b>select</b> :
 * - array("selectkey" => {where-clause})
 * - where-clause evaluted to create select statement
 * <code>
 * $this->select = array(
 *     "all" => "'ORDER BY colname'"
 *     , "byId" => "'WHERE table1.idx=' . \$params['idx'] . '"
 *             . " AND table1.idx=table2.idx"
 *             . " ORDER BY table1.title'"
 *     );
 * </code>
 *
 *
 * <b>tableinfo</b> :
 * - array("tablename" => array("delete","map"))
 * - || array("alias" => array("delete","map"))
 *
 * delete
 * - array("deletekey" => {where-clause})
 * map
 * - array("formfield_name" => "database_name")
 * - || array("formfield_name" => array("column","select","insert","update"))
 *
 * <code>
 * $this->tableinfo = array(
 *     "delete" => array(
 *         "byId" => "'WHERE table1.idx=' . \$params['idx']"
 *         )
 *     , "map" => array(
 *         "idx" => "idx"
 *         , "mod_update" => array("column" => "mod_update"
 *             , "select" => "mod_update"
 *             , "insert" => "NOW()"
 *             , "update" => "NOW()"
 *             )
 *         )
 *     );
 * </code>
 *
 * @package mysql_handler
 */
class MySQLView {
	/**
	 * @var resource returned by MySQLHandler::openConnection()
	 */
	public $dbhandler;

	/**
	 * @var array returned by MySQLView::initMetaData()
	 */
	public $metadata;

	/**
	 * @var array Initialized in subclasses constructor
	 */
	public $tables;
	/**
	 * @var array Initialized in subclasses constructor
	 */
	public $update;
	/**
	 * @var array Initialized in subclasses constructor
	 */
	public $select;
	/**
	 * @var array Initialized in subclasses constructor
	 */
	public $tableinfo;

//	public function __destruct() { echo "<b>".__METHOD__."</b><br/>"; }
//	public function __clone() { echo "<b>".__METHOD__."</b><br/>"; }

	/**
	 * Called after subclass initialization
	 *
	 * Param 'dbhandler' is required : connection resource from MySQLHandler
	 */
	public function MySQLView( $params = array() ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$this->dbhandler = (!empty($params['dbhandler'])) ? $params['dbhandler'] : new MySQLHandler();
		if (!is_object($this->dbhandler))
			{ die("MySQLView ERROR: invalid member 'dbhandler' <br>\n"); }

		if (!is_array($this->tables))
			{ die("MySQLView ERROR: Missing member array 'tables' <br/>\n"); }
//		if (!is_array($this->update))
//			{ die("MySQLView ERROR: Missing member array 'update' <br/>\n"); }
		if (!is_array($this->select))
			{ die("MySQLView ERROR: Missing member array 'select' <br/>\n"); }
		if (!is_array($this->tableinfo))
			{ die("MySQLView ERROR: Missing member array 'tableinfo' <br/>\n"); }

//echo __METHOD__.": <pre>" . $this->toString() . "</pre>\n";
	}

	/**
	 * Populate $this->metadata
	 *
	 * @uses MySQLView::getTablesConfig()
	 * @uses MySQLHandler::listMeta()
	 * @param string &$err    error message, if applicable
	 */
	protected function initMetaData( &$err ) {
//echo "<b>".__METHOD__."</b>()<br/>\n";

		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$tblname = $tblconf['tblname'];

			if (!is_array($this->metadata[$tblname])) {
				if (!$this->dbhandler->listMeta(array("table"=>$tblname),$result,$terr))
					{ $err = __METHOD__." ERROR: $terr"; return false; }

				$this->metadata[$tblname] = array_change_key_case($result,CASE_LOWER);
			}
		}
		reset($this->tables);

//echo __METHOD__.": this->metadata=".print_r($this->metadata,true)."<br/>\n";
		return true;
	}

	/**
	 * Creates SQL for LOAD DATA
	 *
	 * <code>
	 * LOAD DATA [LOW_PRIORITY | CONCURRENT] [LOCAL]
	 *     INFILE 'file_name.txt'
	 *     [REPLACE | IGNORE]
	 *     INTO TABLE tbl_name
	 *     [FIELDS
	 *         [TERMINATED BY '\t']
	 *         [[OPTIONALLY] ENCLOSED BY '']
	 *         [ESCAPED BY '\\' ]
	 *     ]
	 *     [LINES
	 *         [STARTING BY '']
	 *         [TERMINATED BY '\n']
	 *     ]
	 *     [IGNORE number LINES]
	 *     [(col_name,...)]
	 * </code>
	 *
	 * @uses MySQLView::getTablesConfig()
	 * @uses MySQLHandler::listMeta()
	 * @uses MySQLHandler::appendImportColumns()
	 *
	 * @param array $params   posted form fields
	 * @param array &$result  resultset
	 * @param string &$err    error message, if applicable
	 * @return bool           true for success ; false for error
	 */
	public function importView( $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$result = array();
		if (empty($params['INFILE']))
			{ $err = __METHOD__." ERROR: missing param 'INFILE'"; return false; }
		if (count($this->tables) > 1)
			{ $err = __METHOD__." ERROR: importView allowed for single-tables Views only"; return false; }

		if (is_string($params['INFILE'])) {
			$params['INFILE'] = array(
				"path" => $params['INFILE']
				, "fields"   => ','
				, "enclosed" => '"'
				, "escaped"  => '\\\\'
				, "lines"    => '\n'
				, "ignore"   => 0
				);
		}
		if (!is_array($params['INFILE']))
			{ $err = __METHOD__." ERROR: missing param array 'INFILE'"; return false; }

		if (empty($params['INFILE']['fields']))   { $params['INFILE']['fields']   = ','; }
//		if (empty($params['INFILE']['enclosed'])) { $params['INFILE']['enclosed'] = '"'; }
		if (empty($params['INFILE']['escaped']))  { $params['INFILE']['escaped']  = '\\\\'; }
		if (empty($params['INFILE']['lines']))    { $params['INFILE']['lines']    = '\n'; }
		if (empty($params['INFILE']['ignore']))   { $params['INFILE']['ignore']   = 0; }

		if (substr($params['INFILE']['path'],0,1) != "/")
			{ $params['INFILE']['path'] = $this->dbhandler->config->export_path . $params['INFILE']['path']; }

		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$params['INFILE']['tblalias'] = $tblconf['tblalias'];
			$params['INFILE']['tblname'] = $tblconf['tblname'];
		}
		reset($this->tables);

		// build the query
		$query = 'LOAD DATA'
				. (($this->dbhandler->config->load_data_local) ? ' LOCAL' : '')
				. ' INFILE "' . $params['INFILE']['path'] . '"'
				. ' REPLACE'
				. ' INTO TABLE ' . $params['INFILE']['tblname'];

		$query .= ' FIELDS TERMINATED BY \''.$params['INFILE']['fields'].'\'';
		if (!empty($params['INFILE']['enclosed'])) {
			$query .= ' OPTIONALLY ENCLOSED BY \''.$params['INFILE']['enclosed'].'\''
					. ' ESCAPED BY \''.$params['INFILE']['escaped'].'\'';
		}
		$query .= ' LINES TERMINATED BY \''.$params['INFILE']['lines'].'\'';
		if ($params['INFILE']['ignore'] > 0)    // skip first X rows
			{ $query .= ' IGNORE ' . $params['INFILE']['ignore'] . ' LINES'; }

		// COLUMN[, COLUMN]
		$query .= ' (';
		$subparams = array("tblalias" => $params['INFILE']['tblalias']);
		if (!$this->appendImportColumns($subparams,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }
		$query .= ")";

		// run the query
		$this->dbhandler->setDBQuery($query);
		if (!$this->dbhandler->runQuery( $result, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }
//echo __METHOD__.": query=" . htmlentities($query) . "<br/>\n";
//echo __METHOD__.": result=".print_r($result,true)."<br/>\n";

		$this->dbhandler->setDBQuery("OPTIMIZE TABLE " . $params['INFILE']['tblname']);
		if (!$this->dbhandler->runQuery( $optimize_result, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		return true;
	}

	/**
	 * Creates SQL for INSERT
	 *
	 * @uses MySQLView::checkRequiredInsertFields()
	 * @uses MySQLView::getTablesConfig()
	 * @uses MySQLView::appendInsertColumns()
	 * @uses MySQLView::appendInsertValues()
	 * @uses MySQLHandler::setDBQuery()
	 * @uses MySQLHandler::runQuery()
	 *
	 * @param array $params   posted form fields
	 * @param array &$result  resultset
	 * @param string &$err    error message, if applicable
	 * @return bool           true for success ; false for error
	 */
	public function insertView( $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$result = array();
		if (!$this->checkRequiredInsertFields($params,$result,$err))
			{ return false; }

		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$subparams = array(
				"tblalias" => $tblconf['tblalias']
				, "tblname" => $tblconf['tblname']
				, "values" => $params
				);

			// open the query
			$query = 'INSERT INTO ' . $subparams['tblname'] . ' (';

			// COLUMN[, COLUMN]
			if (!$this->appendInsertColumns($subparams,$query,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }

			$query .= ') VALUES (';

			// VALUE[, VALUE]
			if (!$this->appendInsertValues($subparams,$query,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }

			$query .= ")";

			// run the query
			$this->dbhandler->setDBQuery($query);
			if (!$this->dbhandler->runQuery( $result, $terr ))
				{ $err = __METHOD__." ERROR: $terr"; return false; }

//echo __METHOD__.": query=" . htmlentities($query) . "<br/>\n";
//echo __METHOD__.": result=".print_r($result,true)."<br/>\n";
		}
		reset($this->tables);

		return true;
	}

	/**
	 * Creates SQL for REPLACE INTO
	 *
	 * @uses MySQLView::checkRequiredUpdateFields()
	 * @uses MySQLView::appendTablesClause()
	 * @uses MySQLView::appendUpdateValues()
	 * @uses MySQLView::appendLimitClause()
	 * @uses MySQLHandler::setDBQuery()
	 * @uses MySQLHandler::runQuery()
	 *
	 * @param string $updatekey key for array $this->update
	 * @param array $params   posted form fields
	 * @param array &$result  resultset
	 * @param string &$err    error message, if applicable
	 * @return bool           true for success ; false for error
	 */
	public function replaceView( $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$result = array();

		if (!$this->checkRequiredUpdateFields($params,$result,$err))
			{ return false; }

		// open the query
		$query = 'REPLACE INTO ';
		if (!$this->appendTablesClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// SET TABLE.COLUMN = VALUE[, TABLE.COLUMN = VALUE]
		$query .= ' SET ';
		if (!$this->appendReplaceValues($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// run the query
		$this->dbhandler->setDBQuery($query);
		if (!$this->dbhandler->runQuery( $result, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

//echo __METHOD__.": query=" . htmlentities($query) . "<br/>\n";
//echo __METHOD__.": result=".print_r($result,true)."<br/>\n";
		return true;
	}

	/**
	 * Creates SQL for UPDATE
	 *
	 * @uses MySQLView::checkRequiredUpdateFields()
	 * @uses MySQLView::appendTablesClause()
	 * @uses MySQLView::appendUpdateValues()
	 * @uses MySQLView::appendLimitClause()
	 * @uses MySQLHandler::setDBQuery()
	 * @uses MySQLHandler::runQuery()
	 *
	 * @param string $updatekey key for array $this->update
	 * @param array $params   posted form fields
	 * @param array &$result  resultset
	 * @param string &$err    error message, if applicable
	 * @return bool           true for success ; false for error
	 */
	public function updateView( $updatekey, $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>($updatekey,<pre>".print_r($params,true)."</pre>)\n";

		$result = array();
		if (empty($this->update[$updatekey]))
			{ $err = __METHOD__." ERROR: missing member array this->update['$updatekey']"; return false; }

		if (!$this->checkRequiredUpdateFields($params,$result,$err))
			{ return false; }

		// open the query
		$query = 'UPDATE ';
		if (!$this->appendTablesClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// SET TABLE.COLUMN = VALUE[, TABLE.COLUMN = VALUE]
		$query .= ' SET ';
		if (!$this->appendUpdateValues($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// WHERE [ORDER BY]
		eval("\$query .= ' '." . $this->update[$updatekey] . ";");

		// ORDER BY, removed if multi-table update
		if (count($this->tables) > 1 && strpos(strtolower($query)," order by "))
			{ $query = substr($query,0,strpos(strtolower($query)," order by ")); }

		// LIMIT, optional for single-table update
		if (count($this->tables) == 1 && !$this->appendLimitClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// run the query
		$this->dbhandler->setDBQuery($query);
		if (!$this->dbhandler->runQuery( $result, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

//echo __METHOD__.": query=" . htmlentities($query) . "<br/>\n";
//echo __METHOD__.": result=".print_r($result,true)."<br/>\n";
		return true;
	}

	/**
	 * Creates SQL for DELETE
	 *
	 * @uses MySQLView::getTablesConfig()
	 * @uses MySQLView::appendLimitClause()
	 * @uses MySQLHandler::setDBQuery()
	 * @uses MySQLHandler::runQuery()
	 *
	 * @param string $deletekey key for array $this->tableinfo[$tblalias]['delete']
	 * @param array $params   posted form fields
	 * @param array &$result  resultset
	 * @param string &$err    error message, if applicable
	 * @return bool           true for success ; false for error
	 */
	public function deleteView( $deletekey, $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>($deletekey,<pre>".print_r($params,true)."</pre>)\n";

		$result = array();
		$tables = array_reverse($this->tables);
		while (list($k,$val) = each($tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$tblalias = $tblconf['tblalias'];
			$tblname = $tblconf['tblname'];

			if (empty($this->tableinfo[$tblalias]['delete'][$deletekey]))
				{ $err = __METHOD__." ERROR: missing config value this->tableinfo['$tblalias']['delete']['$deletekey']"; return false; }

			// open the query
			$query = 'DELETE FROM ' . $tblname . ' ';

			// WHERE
			eval("\$query .= " . $this->tableinfo[$tblalias]['delete'][$deletekey] . ";");

			// LIMIT, optional for single-table delete
			if (count($this->tables) == 1 && !$this->appendLimitClause($params,$query,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }

			// run the query
			$this->dbhandler->setDBQuery($query);
			if (!$this->dbhandler->runQuery( $result, $err ))
				{ return false; }

//echo __METHOD__.": query=" . htmlentities($query) . "<br/>\n";
//echo __METHOD__.": result=".print_r($result,true)."<br/>\n";
		}
		reset($this->tables);

		return true;
	}

	/**
	 * Creates SQL for SELECT COUNT(*)
	 *
	 * @uses MySQLView::appendTablesClause()
	 * @uses MySQLHandler::setDBQuery()
	 * @uses MySQLHandler::runQuery()
	 *
	 * @param string $selectkey key for array $this->select
	 * @param array $params     select params
	 * @param integer &$result  0..CNT
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	public function countView( $selectkey, $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>($selectkey,<pre>".print_r($params,true)."</pre>)\n";

		$result = -1;
		if (empty($this->select[$selectkey]))
			{ $err = __METHOD__." ERROR: missing config value this->select['$selectkey']"; return false; }

		// open the query
		$query = 'SELECT COUNT(*) AS CNT ';

		// FROM
		$query .= ' FROM ';
		if (!$this->appendTablesClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// WHERE
		eval("\$query .= ' '." . $this->select[$selectkey] . ";");

		// run the query
		$this->dbhandler->setDBQuery($query);
		if (!$this->dbhandler->runQuery( $count, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		$result = 0;
		while (list($key,$val) = each($count)) {
			if (!empty($val['CNT'])) { $result += $val['CNT']; }
		}

//echo __METHOD__.": query=" . htmlentities($query) . "<br/>\n";
//echo __METHOD__.": result=$result <br/>\n";
		return true;
	}

	/**
	 * Creates SQL for SELECT
	 *
	 * @uses MySQLView::appendSelectClause()
	 * @uses MySQLView::appendOutfileClause()
	 * @uses MySQLView::appendTablesClause()
	 * @uses MySQLView::appendLimitClause()
	 * @uses MySQLHandler::setDBQuery()
	 * @uses MySQLHandler::runQuery()
	 *
	 * @param string $selectkey key for array $this->select
	 * @param array $params     select params
	 * @param array &$result    resultset
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	public function selectView( $selectkey, $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>($selectkey,<pre>".print_r($params,true)."</pre>)\n";

		$result = array();
		if (empty($this->select[$selectkey]))
			{ $err = __METHOD__." ERROR: missing config value this->select['$selectkey']"; return false; }

		// open the query
		$query = 'SELECT ';

		// {TBLNAME|TBLALIAS}.COLUMN[, {TBLNAME|TBLALIAS}.COLUMN]
		if (!$this->appendSelectClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// OUTFILE, optional
		if (!$this->appendOutfileClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// FROM
		$query .= ' FROM ';
		if (!$this->appendTablesClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// WHERE [ORDER BY] [GROUP BY]
		eval("\$query .= ' '." . $this->select[$selectkey] . ";");

		// LIMIT, optional
		if (!$this->appendLimitClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// run the query
		$this->dbhandler->setDBQuery($query);
		if (!$this->dbhandler->runQuery( $result, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

//echo __METHOD__.": query=" . htmlentities($query) . "<br/>\n";
//echo __METHOD__.": result=".print_r($result,true)."<br/>\n";
		return true;
	}

	/**
	 * Creates SQL for SELECT XML
	 *
	 * @uses MySQLView::appendSelectXMLClause()
	 * @uses MySQLView::appendTablesClause()
	 * @uses MySQLView::appendLimitClause()
	 * @uses MySQLHandler::setDBQuery()
	 * @uses MySQLHandler::runQuery()
	 *
	 * @param string $selectkey key for array $this->select
	 * @param array $params     select params
	 * @param string &$result   XML or filepath
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	public function selectXML( $selectkey, $params, &$rv, &$err ) {
//echo "<b>".__METHOD__."</b>($selectkey,<pre>".print_r($params,true)."</pre>)\n";

		$rv = '';
		if (empty($this->select[$selectkey]))
			{ $err = __METHOD__." ERROR: missing config value this->select['$selectkey']"; return false; }

		// open the query
		$query = 'SELECT ';

		// CONCAT('<ROW>','<COLUMN>',{TBLNAME|TBLALIAS}.COLUMN,'</COLUMN>','</ROW>') as XML
		if (!$this->appendSelectXMLClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// FROM
		$query .= ' FROM ';
		if (!$this->appendTablesClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// WHERE [ORDER BY] [GROUP BY]
		eval("\$query .= ' '." . $this->select[$selectkey] . ";");

		// LIMIT, optional
		if (!$this->appendLimitClause($params,$query,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// run the query
		$this->dbhandler->setDBQuery($query);
		if (!$this->dbhandler->runQuery( $result, $terr ))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// get non-xml query ; reported in result XML
		$condition = 'SELECT ';
		if (!$this->appendSelectClause($params,$condition,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }
		$condition .= ' FROM ';
		if (!$this->appendTablesClause($params,$condition,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }
		eval("\$condition .= ' '." . $this->select[$selectkey] . ";");
		if (!$this->appendLimitClause($params,$condition,$terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		// format result XML string
		$rv .= "\n".'<RESULT'
			. ' ROWS="' . count($result) . '"'
			. ' VIEW="' . __METHOD__ . '"'
			. ' SELECTKEY="' . $selectkey . '"'
			. '>';
		$rv .= "\n\t".'<QUERY>' . htmlentities($condition) . '</QUERY>';
		while (list($key,$val) = each($result))
			{ $rv .= $val['XML']; }
//			{ $rv .= str_replace("&","&amp;",$val['XML']); }
		$rv .= "\n".'</RESULT>';

		// OUTFILE, optional
		if (!empty($params['OUTFILE'])) {
			$path = __METHOD__ . "_" . date(Ymd_Hi) . ".xml";
			if (is_string($params['OUTFILE']))
				{ $path = $params['OUTFILE']; }
			if (substr($path,0,1) != "/")
				{ $path = $this->dbhandler->config->export_path . $path; }

			$fh = fopen($path,'wb');
			$bytes = fwrite($fh,$rv);
			$rv = "$path ($bytes bytes)";
		}

//echo __METHOD__.": query=" . htmlentities($query) . "<br/>\n";
//echo __METHOD__.": result=<pre>" . htmlentities($result) . "</pre>\n";
		return true;
	}

	/**
	 * Prepare value for insert/update statement
	 *
	 * @param string $tblname
	 * @param string $field
	 * @param string &$value   XML or filepath
	 */
	protected function fixQuotes( $tblname, $field, &$value ) {
//echo "<b>".__METHOD__."</b>($tblname,$field,".htmlentities($value).")<br/>\n";

		$meta = $this->metadata[$tblname][strtolower($field)];
//echo __METHOD__.": ".htmlentities($field)."=".$meta[$this->dbhandler->meta_column_type]."<br/>\n";

		$addquote = (
			!(strpos($meta[$this->dbhandler->meta_column_type],"char") === false)
			|| !(strpos($meta[$this->dbhandler->meta_column_type],"text") === false)
			|| !(strpos($meta[$this->dbhandler->meta_column_type],"blob") === false)
			|| !(strpos($meta[$this->dbhandler->meta_column_type],"time") === false)
			|| !(strpos($meta[$this->dbhandler->meta_column_type],"date") === false)
			|| !(strpos($meta[$this->dbhandler->meta_column_type],"year") === false)
			|| !(strpos($meta[$this->dbhandler->meta_column_type],"enum") === false)
			|| !(strpos($meta[$this->dbhandler->meta_column_type],"set") === false)
			);

		if ($addquote) {
//echo __METHOD__."($tblname,$field,".htmlentities($value).")<br/>\n";

			// demicrosoftize : make some guesses for first round, then remove remaining illegal chars
			$value = strtr(
				$value
				, array(
					"\x82"=>"'","\x83"=>"f","\x84"=>'"',"\x85"=>"...","\x86"=>"*","\x87"=>"*","\x88"=>"^","\x89"=>"^"
					,"\x8a"=>"\xa6","\x8b"=>"<","\x8c"=>"\xbc","\x8e"=>"\xb4"
					,"\x91"=>"'","\x92"=>"'","\x93"=>'"',"\x94"=>'"',"\x95"=>"-","\x96"=>"-","\x97"=>"--","\x98"=>"~","\x99"=>"(tm)"
					,"\x9a"=>"\xa8","\x9b"=>">","\x9c"=>"\xbd","\x9e"=>"\xb8","\x9f"=>"\xbe"
					)
				);
			$value = preg_replace('/[\x00-\x08\x10-\x1F\x80-\x9F]/', '*', $value);

			$value = "'" . addslashes(stripslashes(trim($value))) . "'";
			if ($value == "''") { $value = "'" . strval($meta[$this->dbhandler->meta_default]) . "'"; }
			if ($value == "''") { $value = "NULL"; }
		}
		if (!strlen(strval($value))) { $value = strval($meta[$this->dbhandler->meta_default]); }
		if (!strlen(strval($value))) { $value = "NULL"; }

//echo __METHOD__.": meta=".print_r($meta,true)."<br/>\n";
//echo __METHOD__.": ".htmlentities($field)."=".htmlentities($value)."<br/>\n";
	}

	/**
	 * Check values for required (non null) fields
	 *
	 * @uses MySQLView::getTablesConfig()
	 * @uses MySQLView::fixQuotes()
	 *
	 * @param array $params     posted form fields
	 * @param array &$result    unchanged or same as err
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function checkRequiredInsertFields( $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		if (!$this->initMetaData($terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		$noerr = true;
		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$tblalias = $tblconf['tblalias'];
			$tblname = $tblconf['tblname'];

			if (!is_array($this->tableinfo[$tblalias]['map']))
				{ $err = __METHOD__." ERROR: missing config array this->tableinfo['$tblalias']['map']"; return false; }
			if (!is_array($this->metadata[$tblname]))
				{ $err = __METHOD__." ERROR: missing member array this->metadata['$tblname']"; return false; }

			while (list($key,$meta) = each($this->metadata[$tblname])) {
//echo __METHOD__.": tblname=$tblname, meta=".print_r($meta,true).")<br/>\n";

				$dbname = $meta[$this->dbhandler->meta_column_name];
				$fldname = $this->getFieldName($tblalias, $dbname);

				if (isset($params[$fldname]) && is_array($params[$fldname]))
					{ $params[$fldname] = serialize($params[$fldname]); }
				$this->fixQuotes($tblname, $dbname, $params[$fldname]);
//echo __METHOD__.": ".htmlentities($dbname)."=".htmlentities($params[$fldname]) . "<br/>\n";
				if ($meta[$this->dbhandler->meta_nullable] != $this->dbhandler->meta_nullable_true
						&& strval($params[$fldname]) == 'NULL')
				{
					if ($noerr) {
						$err = "Missing database insert parameter(s): ";
						$noerr = false;
					} else { $err .= ", "; }
					$err .= $fldname;
					$err .= "(".$dbname.")";
				}
			}
			reset($this->metadata[$tblname]);
		}
		reset($this->tables);

		if (!$noerr) {
			$result = array("0" => $err);
			return false;
		}
		return true;
	}

	/**
	 * Check values for required (non null) fields
	 *
	 * @uses MySQLView::getTablesConfig()
	 * @uses MySQLView::fixQuotes()
	 *
	 * @param array $params     posted form fields
	 * @param array &$result    unchanged or same as err
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function checkRequiredUpdateFields( $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		if (!$this->initMetaData($terr))
			{ $err = __METHOD__." ERROR: $terr"; return false; }

		$noerr = true;
		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$tblalias = $tblconf['tblalias'];
			$tblname = $tblconf['tblname'];

			if (!is_array($this->tableinfo[$tblalias]['map']))
				{ $err = __METHOD__." ERROR: missing config array this->tableinfo['$tblalias']['map']"; return false; }
			if (!is_array($this->metadata[$tblname]))
				{ $err = __METHOD__." ERROR: missing member array this->metadata['$tblname']"; return false; }

			while (list($fldname,$dbinfo) = each($this->tableinfo[$tblalias]['map'])) {
				$dbname = $dbinfo;
				if (is_array($dbinfo)) {
					if (empty($dbinfo['column']))
						{ $err = __METHOD__." ERROR: missing config string this->tableinfo['$tblalias']['map']['$fldname']['column']"; return false; }
					if (empty($dbinfo['update']))
						{ continue; }
					$dbname = $dbinfo['column'];
				}
				$meta = $this->metadata[$tblname][strtolower($dbname)];
//echo __METHOD__.": tblname=$tblname, fldname=$fldname, dbname=$dbname, meta=".print_r($meta,true).")<br/>\n";

				if (isset($params[$fldname]) && is_array($params[$fldname]))
					{ $params[$fldname] = serialize($params[$fldname]); }
				$this->fixQuotes($tblname, $dbname, $params[$fldname]);
//echo __METHOD__.": ".htmlentities($dbname)."=".htmlentities($params[$fldname]) . "<br/>\n";
				if ($meta[$this->dbhandler->meta_nullable] != $this->dbhandler->meta_nullable_true
						&& strval($params[$fldname]) == 'NULL')
				{
					if ($noerr) {
						$err = "Missing database update parameter(s): ";
						$noerr = false;
					} else { $err .= ", "; }
					$err .= $fldname;
					$err .= "(".$dbname.")";
				}
			}
			reset($this->tableinfo[$tblalias]['map']);
		}
		reset($this->tables);

		if (!$noerr) {
			$result = array("0" => $err);
			return false;
		}
		return true;
	}

	/**
	 * @param array $params     $this->tableinfo[$params['tblalias']]['map']
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendImportColumns( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		if (empty($params['tblalias']))
			{ $err = __METHOD__." ERROR: missing param 'tblalias'"; return false; }

		$addcomma = false;
		while (list($fldname,$dbinfo) = each($this->tableinfo[$params['tblalias']]['map'])) {
			$dbname = $dbinfo;
			if (is_array($dbinfo))
				{ $dbname = $dbinfo['column']; }

			if (!$addcomma) { $addcomma = true; }
			else { $query .= ", "; }

			$query .= $dbname;
		}
		reset($this->tableinfo[$params['tblalias']]['map']);

		return true;
	}

	/**
	 * @param array $params     $this->tableinfo[$params['tblalias']]['map']
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendInsertColumns( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		if (empty($params['tblalias']))
			{ $err = __METHOD__." ERROR: missing param 'tblalias'"; return false; }

		$addcomma = false;
		while (list($fldname,$dbinfo) = each($this->tableinfo[$params['tblalias']]['map'])) {
			$dbname = $dbinfo;
			if (is_array($dbinfo)) {
				if (empty($dbinfo['insert'])) { continue; }
				$dbname = $dbinfo['column'];
			}

			if (!$addcomma) { $addcomma = true; }
			else { $query .= ", "; }

			$query .= $dbname;
		}
		reset($this->tableinfo[$params['tblalias']]['map']);

		return true;
	}

	/**
	 * @param array $params 3 required params
	 * - string tblalias
	 * - string tblname
	 * - array values
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendInsertValues( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		if (empty($params['tblalias']))
			{ $err = __METHOD__." ERROR: missing param 'tblalias'"; return false; }
		if (empty($params['tblname']))
			{ $err = __METHOD__." ERROR: missing param 'tblname'"; return false; }
		if (!is_array($params['values']))
			{ $err = __METHOD__." ERROR: missing param 'values'"; return false; }

		$addcomma = false;
		while (list($fldname,$dbinfo) = each($this->tableinfo[$params['tblalias']]['map'])) {
			// substitute 'dbname' for fldname within config select string
			$dbinsert = $dbname = $dbinfo;
			if (is_array($dbinfo)) {
				if (empty($dbinfo['insert'])) { continue; }
				$dbname = $dbinfo['column'];
				$dbinsert = $dbinfo['insert'];
			}
			if (isset($params['values'][$fldname]) && is_array($params['values'][$fldname]))
				{ $params['values'][$fldname] = serialize($params['values'][$fldname]); }
			$this->fixQuotes($params['tblname'], $dbname, $params['values'][$fldname]);
			$dbinsert = str_replace($dbname, $params['values'][$fldname], $dbinsert);

			if (!$addcomma) { $addcomma = true; }
			else { $query .= ", "; }

			$query .= $dbinsert;
		}
		reset($this->tableinfo[$params['tblalias']]['map']);

		return true;
	}

	/**
	 * Iterate $this->tables
	 *
	 * @uses MySQLView::getTablesConfig()
	 *
	 * @param array $params     none
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendTablesClause( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$addcomma = false;
		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$tblalias = $tblconf['tblalias'];
			$tblname = $tblconf['tblname'];
			$join_type = $tblconf['join_type'];
			$join_condition = $tblconf['join_condition'];

			if (!empty($join_type) || !empty($join_condition)) {
				$query .= " $join_type JOIN ";
				$query .= $tblname;
				if ($tblname != $tblalias)   { $query .= " $tblalias"; }
				if (!empty($join_condition)) {
					eval("\$query .= ' ON '." . $join_condition . ";");
				}

			} else {
				if ($addcomma) { $query .= ", "; }
				$query .= $tblname;
				if ($tblname != $tblalias)   { $query .= " $tblalias"; }
			}
			$addcomma = true;
		}
		reset($this->tables);

		return true;
	}

	/**
	 * Iterate $this->tableinfo
	 * Update all columns with params
	 *
	 * @uses MySQLView::getTablesConfig()
	 * @uses MySQLView::fixQuotes()
	 *
	 * @param array $params     posted form fields
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendReplaceValues( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$addcomma = false;
		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$tblalias = $tblconf['tblalias'];
			$tblname = $tblconf['tblname'];

			while (list($fldname,$dbinfo) = each($this->tableinfo[$tblalias]['map'])) {
				$dbupdate = $dbname = $dbinfo;
				if (is_array($dbinfo))
					{ $dbname  = $dbinfo['column']; }
				if (isset($params[$fldname]) && is_array($params[$fldname]))
					{ $params[$fldname] = serialize($params[$fldname]); }
				$this->fixQuotes($tblname, $dbname, $params[$fldname]);
				$dbupdate = $params[$fldname];

				if (!$addcomma) { $addcomma = true; }
				else { $query .= ", "; }

				$query .= "$tblname.$dbname=$dbupdate";

			}
			reset($this->tableinfo[$tblalias]['map']);
		}
		reset($this->tables);

		return true;
	}

	/**
	 * Iterate $this->tableinfo
	 * Update columns with view's config + params
	 *
	 * @uses MySQLView::getTablesConfig()
	 * @uses MySQLView::fixQuotes()
	 *
	 * @param array $params     posted form fields
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendUpdateValues( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$addcomma = false;
		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$tblalias = $tblconf['tblalias'];
			$tblname = $tblconf['tblname'];

			while (list($fldname,$dbinfo) = each($this->tableinfo[$tblalias]['map'])) {
				$dbupdate = $dbname = $dbinfo;
				if (is_array($dbinfo)) {
					if (empty($dbinfo['update'])) { continue; }

					$dbname = $dbinfo['column'];
					$dbupdate = $dbinfo['update'];
				}
				if (isset($params[$fldname]) && is_array($params[$fldname]))
					{ $params[$fldname] = serialize($params[$fldname]); }
				$this->fixQuotes($tblname, $dbname, $params[$fldname]);
				$dbupdate = str_replace($dbname, $params[$fldname], $dbupdate);

				if (!$addcomma) { $addcomma = true; }
				else { $query .= ", "; }

				$query .= "$tblname.$dbname=$dbupdate";

			}
			reset($this->tableinfo[$tblalias]['map']);
		}
		reset($this->tables);

		return true;
	}

	/**
	 * Iterate $this->tableinfo
	 *
	 * @param array $params     none
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendSelectClause( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$addcomma = false;
		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$tblalias = $tblconf['tblalias'];
			$tblname = $tblconf['tblname'];

			if (!is_array($this->tableinfo[$tblalias]['map']))
				{ $err = __METHOD__." ERROR: missing config array this->tableinfo['$tblalias']['map']"; return false; }

			while (list($fldname,$dbinfo) = each($this->tableinfo[$tblalias]['map'])) {
				// substitute 'dbname' for fldname within config select string
				$dbselect = $dbname = $dbinfo;    // if (is_string($dbinfo))
				if (is_array($dbinfo)) {
					if (empty($dbinfo['column']))
						{ $err = __METHOD__." ERROR: missing config string this->tableinfo['$tblalias']['map']['$fldname']['column']"; return false; }
					if (empty($dbinfo['select']))
						{ continue; }

					$dbname = $dbinfo['column'];
					$dbselect = $dbinfo['select'];
				}

				$dbselect = str_replace($dbname, $tblalias.".".$dbname, $dbselect);

				if (!$addcomma) { $addcomma = true; }
				else { $query .= ", "; }

				$query .= $dbselect;
				if ($dbname != $fldname) { $query .= " as $fldname"; }

			}
			reset($this->tableinfo[$tblalias]['map']);
		}
		reset($this->tables);

		return true;
	}

	/**
	 * Iterate $this->tables, $this->tableinfo.
	 * Selects <fldname>value</fldname>
	 *
	 * @param array $params     none
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendSelectXMLClause( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$query .= "CONCAT('\n\t<ROW>'";

		while (list($k,$val) = each($this->tables)) {
			if (!$this->getTablesConfig($val,$tblconf,$terr))
				{ $err = __METHOD__." ERROR: $terr"; return false; }
			$tblalias = $tblconf['tblalias'];
			$tblname = $tblconf['tblname'];

			if (!is_array($this->tableinfo[$tblalias]['map']))
				{ $err = __METHOD__." ERROR: missing config array this->tableinfo['$tblalias']['map']"; return false; }

			while (list($fldname,$dbinfo) = each($this->tableinfo[$tblalias]['map'])) {
				// substitute 'dbname' for fldname within config select string
				$dbselect = $dbname = $dbinfo;    // if (is_string($dbinfo))
				if (is_array($dbinfo)) {
					if (empty($dbinfo['column']))
						{ $err = __METHOD__." ERROR: missing config string this->tableinfo['$tblalias']['map']['$fldname']['column']"; return false; }
					if (empty($dbinfo['select']))
						{ continue; }

					$dbname = $dbinfo['column'];
					$dbselect = $dbinfo['select'];
				}

				$dbselect = str_replace($dbname, $tblalias.".".$dbname, $dbselect);

				$query .= ", '\n\t\t<" . ($fldname) . ">',"
					. "IF (" . $dbselect . " IS NOT NULL,"
//					. $dbselect
					. "REPLACE(" . $dbselect . ",'&','&amp;')"
//					. "REPLACE(REPLACE(REPLACE(" . $dbselect . ",'&','&amp;'),'<','&lt;'),'>','&gt;')"
					. ",'')"
					. ", '</" . ($fldname) . ">'";
			}
			reset($this->tableinfo[$tblalias]['map']);
		}
		reset($this->tables);

		$query .= ", '\n\t</ROW>') as XML ";

		return true;
	}

	/**
	 * Directs query result to $this->dbhandler->config->export_path . $params['OUTFILE']['path']
	 * - comma-delimited : default, csv format
	 * - tab-delimited   : $params['OUTFILE'] = array("path" => 'filename', "fields" => '\t', "lines" => '\n');
	 *
	 * @param array $params Returns if OUTFILE param is empty
	 * - integer OUTFILE : 1 == true
	 * - string OUTFILE  : filepath
	 * - array OUTFILE   : path ; fields ; lines ; enclosed
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendOutfileClause( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		if (empty($params['OUTFILE'])) { return true; }

		if ($params['OUTFILE'] == 1)
			{ $params['OUTFILE'] = array("enclosed" => '"'); }
		if (is_string($params['OUTFILE'])) {
			$params['OUTFILE'] = array(
				"path" => $params['OUTFILE']
				, "fields"   => ','
				, "enclosed" => '"'
				, "lines"    => '\n'
				);
		}
		if (is_array($params['OUTFILE'])) {
			if (empty($params['OUTFILE']['path']))     { $params['OUTFILE']['path'] = __METHOD__ . "_" . date(Ymd_Hi) . ".csv"; }
			if (empty($params['OUTFILE']['fields']))   { $params['OUTFILE']['fields']   = ','; }
//			if (empty($params['OUTFILE']['enclosed'])) { $params['OUTFILE']['enclosed'] = '"'; }
			if (empty($params['OUTFILE']['lines']))    { $params['OUTFILE']['lines']    = '\n'; }

			if (substr($params['OUTFILE']['path'],0,1) != "/")
				{ $params['OUTFILE']['path'] = $this->dbhandler->config->export_path . $params['OUTFILE']['path']; }
		}

		$query .= " INTO OUTFILE \"".$params['OUTFILE']['path']."\"";
		$query .= " FIELDS TERMINATED BY '".$params['OUTFILE']['fields']."'";
		if (!empty($params['OUTFILE']['enclosed']))
			{ $query .= " OPTIONALLY ENCLOSED BY '".$params['OUTFILE']['enclosed']."'"; }
		$query .= " LINES TERMINATED BY '".$params['OUTFILE']['lines']."'";

		return true;
	}

	/**
	 * Appends LIMIT(offset,count) to query
	 *
	 * @param array $params Returns if LIMIT param is empty
	 * - integer LIMIT : max rows
	 * - array LIMIT   : offset ; rowcount
	 * @param array &$query     query fragment
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	protected function appendLimitClause( $params, &$query, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		if (empty($params['LIMIT'])) { return true; }

		if (is_numeric($params['LIMIT'])) {
			$params['LIMIT'] = array(
				"offset" => 0
				, "rowcount" => $params['LIMIT']
				);
		}
		if (is_array($params['LIMIT'])) {
			if (empty($params['LIMIT']['offset']) || $params['LIMIT']['offset'] < 0)
				{ $params['LIMIT']['offset'] = 0; }
			if (empty($params['LIMIT']['rowcount']) || $params['LIMIT']['rowcount'] < 0)
				{ $params['LIMIT']['rowcount'] = 1; }
			$query .= " LIMIT ".$params['LIMIT']['offset'].",".$params['LIMIT']['rowcount'];
		}
		return true;
	}

	/**
	 * @param array $params     array element of $this->tables
	 * @param array &$result    tblalias ; tblname ; join_type ; join_condition
	 * @param string &$err      error message, if applicable
	 * @return bool             true for success ; false for error
	 */
	public function getTablesConfig( $params, &$result, &$err ) {
//echo "<b>".__METHOD__."</b>(<pre>".print_r($params,true)."</pre>)\n";

		$result = array();
		$err = "";

		// option one
		if (is_string($params)) {
			$result = array(
				"tblalias" => $params
				, "tblname" => $params
				, "join_type" => ""
				, "join_condition" => ""
				);

		// option two
		} else if (is_array($params) && count($params) == 1) {
			while (list($alias,$name) = each($params)) {
				$result = array(
					"tblalias" => $alias
					, "tblname" => $name
					, "join_type" => ""
					, "join_condition" => ""
					);
				break;
			}

		// option three
		} else {
			if (empty($params['tblalias']))       { $params['tblalias'] = null; }
			if (empty($params['tblname']))        { $params['tblname'] = null; }
			if (empty($params['join_type']))      { $params['join_type'] = null; }
			if (empty($params['join_condition'])) { $params['join_condition'] = null; }

			if (empty($params['tblalias']))
				{ $err = __METHOD__." ERROR: missing config string 'tblalias'"; return false; }
			if (empty($params['tblname']))
				{ $err = __METHOD__." ERROR: missing config string 'tblname'"; return false; }
			if (empty($params['join_type']) && !empty($params['join_condition']))
				{ $params['join_type'] = "INNER"; }

			$result = array(
				"tblalias" => $params['tblalias']
				, "tblname" => $params['tblname']
				, "join_type" => $params['join_type']
				, "join_condition" => $params['join_condition']
				);
		}

//echo __METHOD__.": result=".print_r($result,true)."<br/>\n";
		return true;
	}

	/**
	 * Get field name corrsponding to column name
	 *
	 * @param string $tblinfo   key for $this->tableinfo array
	 * @param string $dbname    database column name
	 * @return string           corresponding params/formfield name
	 */
	protected function getFieldName( $tblinfo, $dbname ) {
//echo "<b>".__METHOD__."</b>($tblinfo,$dbname)<br/>\n";

		$result = "";
		while (list($fldname,$dbinfo) = each($this->tableinfo[$tblinfo]['map'])) {
			$curname = $dbinfo;
			if (is_array($dbinfo))
				{ $curname = $dbinfo['column']; }

			if (strtolower($curname) == strtolower($dbname)) {
				$result = $fldname;
				break;
			}
		}
		reset($this->tableinfo[$tblinfo]['map']);
		return $result;
	}

	/**
	 * Get column name corrsponding to field name
	 *
	 * @param string $tblinfo   key for $this->tableinfo array
	 * @param string $fldname   key for $this->tableinfo[$tblinfo]['map'] array
	 * @return string           corresponding column name
	 */
	protected function getDBName( $tblinfo, $fldname ) {
//echo "<b>".__METHOD__."</b>($tblinfo,$fldname)<br/>\n";

		$dbinfo = $this->tableinfo[$tblinfo]['map'][$fldname];

		$result = $dbinfo;
		if (is_array($dbinfo))
			{ $result = $dbinfo['column']; }

		return $result;
	}

	/**
	 * Return class methods and members
	 * @return string
	 */
	public function toString() {
		ob_start();                   // start output buffering

		echo "<b>BEGIN ".__METHOD__."</b> ( \n";
		echo "\n<b>class_methods</b> (".print_r(get_class_methods($this),true).")";
		echo "\n<b>class_members</b> (".print_r($this,true).")";
		echo "\n) <b>END ".__METHOD__."</b> \n";

		$result = ob_get_contents();  // get buffer content
		ob_end_clean();               // stop output buffering

		return $result;               // return content
	}

}

</script>
