<?php

function test_ketama() {
	$ketama = new Redis_Ketama();
    $ketama->add_server("10.0.1.1", 11211, 600);
    $ketama->add_server("10.0.1.2", 11211, 300);
    $ketama->add_server("10.0.1.3", 11211, 200);
    $ketama->add_server("10.0.1.4", 11211, 350);
    $ketama->add_server("10.0.1.5", 11211, 1000);
    $ketama->add_server("10.0.1.6", 11211, 800);
    $ketama->add_server("10.0.1.7", 11211, 950);
    $ketama->add_server("10.0.1.8", 11211, 100);
    
    $ketama->create_continuum();
    
    $ordinal = $ketama->get_server("piet5234");
	echo "ord: ", $ordinal, PHP_EOL;
	echo "addr: ", $ketama->get_server_addr($ordinal), PHP_EOL;
}


//simple connection manager
function get_connection($addr)
{
    static $connections = array();
    if(!isset($connections[$addr])) {
        $connections[$addr] = new Redis_Connection($addr);
    } 
    return $connections[$addr];  
}

function mget($keys, $ketama) {
	$batches = array();
	$batch_keys = array();
    //add all keys to batches
    foreach($keys as $key) {
        $server_ordinal = $ketama->get_server($key);
        if(!isset($batches[$server_ordinal])) {
            $batch = new Redis_Batch("MGET");
            $batches[$server_ordinal] = $batch;
    	}
    	else {
    		$batch = $batches[$server_ordinal];
		}
        $batch->write(" $key");
        $batch_keys[$server_ordinal][] = $key;
    }
    //finalize batches, and start executing
    foreach($batches as $server_ordinal=>$batch) {
        $batch->write("\r\n", 1);
        $server_addr = $ketama->get_server_addr($server_ordinal);
		$connection = get_connection($server_addr);	
        $connection->execute($batch);
    }
    //handle events until all complete
	Redis_dispatch();
    //build up results
    $results = array();
    foreach($batches as $server_ordinal=>$batch) {
    	//read multibulk reply
	    $batch->next_reply(&$mb_reply_type, &$mb_reply_value, &$mb_reply_length);
	    foreach($batch_keys[$server_ordinal] as $key) {
    	    $batch->next_reply(&$reply_type, &$reply_value, &$reply_length);
    	    $results[$key] = $reply_value;
		}
	}
	
	return $results;
}

function test_mget()
{
    $ketama = new Redis_Ketama();
    $ketama->add_server("127.0.0.1", 6379, 100);
    $ketama->add_server("127.0.0.1", 6380, 100);
    $ketama->create_continuum();

    //$N = 200000;
    $N = 20000;
    $M = 200;
    
    $connection1 = get_connection("127.0.0.1:6379");;
    $connection2 = get_connection("127.0.0.1:6380");
    $keys = array();
    for($i = 0; $i < $M; $i++) {
        $key = "piet$i";
        $value = "blaat$i";
        $len = strlen($value);
        $batch = new Redis_Batch("SET $key $len\r\n$value\r\n", 1);
        $connection1->execute($batch, true);
        $batch = new Redis_Batch("SET $key $len\r\n$value\r\n", 1);
        $connection2->execute($batch, true);
        $keys[] = $key;
    }
    
    $start = microtime(true);
    for($i = 0; $i < $N; $i++) {
        mget($keys, $ketama);
        //print_r(mget($keys, $ketama));
    }
    $end = microtime(true);
    echo "per/sec ", $N * $M / ($end - $start), PHP_EOL;
    echo "msec per mget ", ($end - $start) / $N * 1000.0, PHP_EOL;
}

function _test_simple($connection, $key)
{
	$batch = new Redis_Batch("GET $key\r\n", 1);
	$connection->execute($batch, true);
	$batch->next_reply(&$reply_type, &$reply_value, &$reply_length);
	return $reply_value;
}

function test_simple() {
	
	$connection = new Redis_Connection("127.0.0.1:6379");
	
	for($i = 0; $i < 100; $i++) {
		_test_simple($connection, "library");
	}
}

function test_destroy() {
    //$connection = new Redis_Connection("127.0.0.1:6379");
    $connection = get_connection("127.0.0.1:6379");
    $connection = get_connection("127.0.0.1:6379");
    $connection = get_connection("127.0.0.1:6379");
}

function test_integer_reply()
{
	$batch = new Redis_Batch();
    $batch->write("INCR incr_test\r\n");	
    $batch->finalize(1);
	$connection = new Redis_Connection("127.0.0.1:6379");
	$connection->execute($batch);
	Redis_dispatch();
	while($level = $batch->next_reply(&$reply_type, &$reply_value, &$reply_length)) {
		echo "start", PHP_EOL;
		echo "\ttype ", $reply_type, PHP_EOL;
		echo "\tval ", $reply_value, " (", gettype($reply_value), ")", PHP_EOL;
		echo "\tlen ", $reply_length, PHP_EOL;
		echo "\tlevel ", $level, PHP_EOL;
		echo "end", PHP_EOL;
	}
}

//test_ketama();
//test_simple();
test_mget();
//test_destroy();
//test_integer_reply();
//echo "done...!", PHP_EOL;
?>