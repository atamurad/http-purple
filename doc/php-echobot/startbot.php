<?php 

$args = array(
  'method'    => 'login',
  // Edit 4 lines below
  'apikey'    => '[API KEY]',
  'prpl'      => '[IM Protocol]',
  'account'   => '[IM User account]',
  'password'  => '[IM Password]);

$url = 'http://apiim01.atamurad.com/api';

$c = curl_init();
curl_setopt($c, CURLOPT_URL, $url);
curl_setopt($c, CURLOPT_POSTFIELDS, $args);
curl_setopt($c, CURLOPT_RETURNTRANSFER, 1);
$result = json_decode(curl_exec($c));
curl_close($c);

// print_r($result);

if( $result->result == "Success" )
  echo "Your bot is starting!";
else
  echo "Error! ".$result->error_msg;

?>
