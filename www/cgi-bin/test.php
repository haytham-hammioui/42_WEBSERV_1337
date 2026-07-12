<?php
echo "Content-Type: text/html\r\n";
echo "\r\n";
echo "<html><body>";
echo "<h1>PHP CGI Test</h1>";
echo "<p>Time: " . date('Y-m-d H:i:s') . "</p>";
echo "<p>Method: " . $_SERVER['REQUEST_METHOD'] . "</p>";
echo "<p>Script: " . $_SERVER['SCRIPT_NAME'] . "</p>";
echo "</body></html>";
?>