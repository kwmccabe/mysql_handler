<?php
/**
 * @package mysql_handler
 * @copyright Copyright © 2001-2017 Ten&Two Systems. All Rights Reserved.
 *
 * command-line equivalent:
 * mysql --host={dbhost} --database={dbname} --user={dbuser} --pass={dbpass}
 *
 * mysqldump {dbname} --host={dbhost} --user={dbuser} --password={dbpass} --no-data > {dbhost}_{dbname}_tables.sql
 */

/**
 * @see MySQLHandler
 */
require_once("mysql_handler/MySQLHandler.inc");

/**
 * @see MySQLView
 */
require_once("mysql_handler/MySQLView.inc");

/**
 * Define primary database connection
 * and options for MySQLHandler and MySQLView.
 *
 * @see MySQLView
 * @see MySQLHandler
 * @package mysql_handler
 */
class MySQLHandlerConfig {
	/**
	 * @see MySQLHandler::openConnection()
	 * @var string MySQL host name, eg, 'localhost' , 'www.mydomain.com' , '12.34.56.78:4444'
	 */
	public $dbhost;

	/**
	 * @see MySQLHandler::openConnection()
	 * @var string MySQL database name
	 */
	public $dbname;

	/**
	 * @see MySQLHandler::openConnection()
	 * @var string MySQL database user's name
	 */
	public $dbuser;

	/**
	 * @see MySQLHandler::openConnection()
	 * @var string MySQL database user's password
	 */
	public $dbpass;

	/**
	 * @see MySQLHandler::runQuery()
	 * @see MySQLHandler::closeConnection()
	 * @var bool Automatically close db connections after each query
	 */
	public $close_connections;

	/**
	 * @see MySQLView::importView()
	 * @var bool True (LOAD DATA LOCAL INFILE) || False (LOAD DATA INFILE)
	 */
	public $load_data_local;

	/**
	 * @see MySQLView::appendOutfileClause()
	 * @see MySQLView::importView()
	 * @see MySQLView::selectXML()
	 * @var string Default path for file operations
	 */
	public $export_path;

	/**
	 * Values for each config option are set here.
	 */
	function MySQLHandlerConfig() {
		$this->dbhost = "localhost";
		$this->dbname = "dbname";
		$this->dbuser = "dbuser";
		$this->dbpass = "dbpass";

		$this->close_connections = false;
		$this->load_data_local   = false;
		$this->export_path       = "/var/data/mysql/";  // with trailing '/'
	}

}  // end class MySQLHandlerConfig
