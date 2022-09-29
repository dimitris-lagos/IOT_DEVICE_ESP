    var Socket;
    function init() {
      Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
      Socket.onmessage = function(event){
        document.getElementById("rxConsole").value += event.data;
      }
    }
    function sendText(){
      Socket.send(document.getElementById("txBar").value);
      document.getElementById("txBar").value = "";
    }
    function sendBrightness(){
      Socket.send("#"+document.getElementById("brightness").value);
    }
    function xml_handler(){
      console.log("button was clicked!");
      var xhr = new XMLHttpRequest();
      var url = "/xml";
      xhr.onreadystatechange = function() {
        if(this.readyState == 4 && this.status == 200) {
          var xmlDoc =this.responseXML;
          var x = xmlDoc.getElementsByTagName("status");
          y=x[0].getElementsByTagName("mode")[0].childNodes[0].nodeValue
          z=x[0].getElementsByTagName("ip")[0].childNodes[0].nodeValue
          document.getElementById("Status").innerHTML = y;
          document.getElementById("Ip").innerHTML = z;
          }
        };
      xhr.open("GET", url, true);
      xhr.send();
    };
document.addEventListener('DOMContentLoaded', xml_handler, false); 