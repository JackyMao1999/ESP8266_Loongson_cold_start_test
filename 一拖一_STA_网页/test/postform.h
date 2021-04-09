#ifndef _POSTFORM_H_
#define _POSTFORM_H_
#include <Arduino.h>

//const String postForms = "<html>
//  <head>
//    <title>ESP8266 Web Server POST handling</title>
//    <style>
//      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }
//    </style>
//  </head>
//  <body>
//    <h1>POST plain text to /postplain/</h1><br>
//    <form method="post" enctype="text/plain" action="/postplain/">
//      <input type="text" name='{"hello": "world", "trash": "' value='"}'><br>
//      <input type="submit" value="Submit">
//    </form>
//    <h1>POST form data to /postform/</h1><br>
//    <form method="post" enctype="application/x-www-form-urlencoded" action="/postform/\\">\
//      <input type=\\"text\" name=\"hello\" value=\"world\"><br>\
//      <input type=\"submit\" value=\"Submit\">\
//    </form>\
//  </body>\
//</html>";
const String postForms = "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\
<html>\
  <head>\
    <title>A1915 Cold Power Test</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\
    </style>\
  </head>\
  <body>\
    <style>\
      .coms.div{\
        display: inline-block;\
      }\
     .blocks {\
      text-align: center;\
      border-width:2px;\
      border-style:solid;\
      border-color:black;\
      margin-bottom: 5px;\
     }\
    </style>\
    <div>\
        <h1 style=\"text-align: center\">A1915 Cold Power Test</h1>\
        <span id=\"datetime\" style=\"position:fixed;top: 30px;right: 5px;\">NA</span>\
    </div>\
    <div class=\"coms\">\
      <div class=\"blocks\">\
        <div class=\"info\" style=\"float: left\">\
          <p>AC通电延时时间:<span id=\"StartDelay_T\">NA</p>\
          <p>AC通电持续时间:<span id=\"Start_T\">NA</p>\
          <p>AC通电冷却时间:<span id=\"CloseDelay_T\">NA</p>\
          <p>ESP Uart: <span id=\"uart\">NA</p>\
          <p>PC State: <span id=\"state\">NA</p>\
        </div>\
        <div class=\"forms\">\
          <form action=\"/postform\">\
            AC通电延时时间<br>\
            <input type=\"number\" name='StartDelay_T' value=10><br>\
            AC通电持续时间<br>\
            <input type=\"number\" name='Start_T' value=5><br>\
            AC通电冷却时间<br>\
            <input type=\"number\" name='CloseDelay_T' value=5><br>\
            <input type=\"submit\" value=\"Send\">\
          </form>\
        </div>\
        <div class=\"controls\">\
          <button class=\"button\" onclick=\"send(1)\">Start</button>\
          <button class=\"button\" onclick=\"send(0)\">Stop</button>\
        </div>\
      </div>\
      <div class=\"blocks\">\
        <p>PC Uart: <span id=\"uart\">NA</p>\
        <p>PC State: <span id=\"state\">NA</p>\
        <button class=\"button\" onclick=\"send(1)\">Start</button>\
        <button class=\"button\" onclick=\"send(0)\">Stop</button>\
      </div>\
    </div>\
  </body>\
  <script>\
    function send(pc_set)\
    {\
      var xhttp = new XMLHttpRequest();\
      xhttp.onreadystatechange = function() {\
        if (this.readyState == 4 && this.status == 200) {\
          document.getElementById(\"state\").innerHTML = this.responseText;\
        }\
      };\
      xhttp.open(\"GET\", \"PC_set?pc_set=\"+pc_set, true);\
      xhttp.send();\
    }\
    setInterval(function()\
    {\
        document.getElementById(\"datetime\").innerHTML = new Date().toLocaleString();\
        getUart();\
        getACstate();\
    }, 1000);\
    function getUart() {\
      var xhttp = new XMLHttpRequest();\
      xhttp.onreadystatechange = function() {\
        if (this.readyState == 4 && this.status == 200) {\
          document.getElementById(\"uart\").innerHTML = this.responseText;\
        }\
      };\
      xhttp.open(\"GET\", \"uart\", true);\
      xhttp.send();\
    }\
    function getACstate() {\
      var xhttp = new XMLHttpRequest();\
      xhttp.onreadystatechange = function() {\
        if (this.readyState == 4 && this.status == 200) {\
          var actime = this.responseText.split(',');\
          document.getElementById(\"StartDelay_T\").innerHTML = actime[0];\
          document.getElementById(\"Start_T\").innerHTML = actime[1];\
          document.getElementById(\"CloseDelay_T\").innerHTML = actime[2];\
        }\
      };\
      xhttp.open(\"GET\", \"ACtime\", true);\
      xhttp.send();\
    }\
  </script>\
</html>";

#endif
