<?php

// Send HTTP request
function call_method($data) 
{
  // Copy prpl, account and apikey from request
  $data['prpl'] = $_POST['prpl'];
  $data['account'] = $_POST['account'];
  $data['apikey'] = $_POST['apikey'];

  $url = 'http://apiim01.atamurad.com/api';

  $c = curl_init();
  curl_setopt($c, CURLOPT_URL, $url);
  curl_setopt($c, CURLOPT_POSTFIELDS, $data);
  $result = curl_exec($c);
  curl_close($c);
}

/* Reply to IM message */
function reply_im($msg)
{
  $args = array(
    'method'  => 'send_im',
    'to'      => $_POST['from'],
    'message' => $msg);

  call_method($args);
}

/* Change account status */
function change_status($status, $message)
{
  $args = array(
    'method' => 'change_status',
    'status' => $status,
    'message' => $message);

  call_method($args);
}

$event = $_POST['event'];

if( $event == 'connected') { 
  change_status('away', 'EchoBot 1.0 is running! Waiting for your messages :)');
}

if( $event == 'message' ) {   // IM received

  $from = $_POST['from'];
  $msg = $_POST['message'];

  $reply = "Hello, ".$from."! You just said ".$msg;

  reply_im($reply);
}

?>
