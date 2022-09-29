const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta http-equiv="X-UA-Compatible" content="IE=edge">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP8266 WebServer</title>
  <link rel="icon" type="image/x-icon" href="/favicon.ico">
  <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.7.1/chart.min.js"
    integrity="sha512-QSkVNOCYLtj73J4hbmVoOV6KVZuMluZlioC+trLpewV8qMjsWqlIQvkn1KGX2StWvPMdWGBqim1xlC8krl1EKQ=="
    crossorigin="anonymous" referrerpolicy="no-referrer"></script>
  <script src="https://cdn.jsdelivr.net/npm/moment@2.27.0"></script>
  <script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-moment@0.1.1"></script>
  <script>
    var Socket;
    var ctx;
    var myChart;
    var xAxis = [];
    var yAxis = [];
    var temperatureData = [];
    async function getData() {
      const response = await fetch('datalog.txt');
      const data = await response.text();
      const table = data.split('\n');
      var time;
      var counter = 0;
      table.forEach(row => {
        if (row.startsWith("time:")) {
          time = parseInt(row.substr(row.lastIndexOf(":") + 1, row.length)) * 1000;
          counter = 0;
        }
        else if (row != "") {
          let columns = row.split(',');
          time = time + 60000;
          let date = new Date(+time);
          temperatureData.push({ x: date, y: parseFloat(columns[0]) });
        }
      });
      manualDecimate();
    }
    function manualDecimate() {
      var selectedValue = parseFloat(temperatureData[0].y);
      var selectedIndex = 0;
      var counter = 0;
      while (selectedIndex + 1 < temperatureData.length) {
        counter = 0;
        while (((selectedIndex + counter + 1) < temperatureData.length) && selectedValue == parseFloat(temperatureData[selectedIndex + counter].y)) {
          counter++;
        }
        selectedIndex++;
        temperatureData.splice(selectedIndex, counter - 1);
        selectedValue = parseFloat(temperatureData[selectedIndex].y);
      }
    }
    const skipped = (ctx, value) => ctx.p0.skip || ctx.p1.skip ? value : undefined;
    function formater(value) {
      let format = value;
      if (parseInt(value) < 10) {
        a
        format = "0" + value;
      }
      return format;
    }
    async function createChart() {
      await getData();
      ctx = document.getElementById('myChart').getContext('2d');
      let gradient = ctx.createLinearGradient(0, 0, 0, 400);
      gradient.addColorStop(0, 'rgba(58,123,213,1)');
      gradient.addColorStop(1, 'rgba(0,210,255,0.3)');

      var data = {
        datasets: [{
          label: 'Temperature',
          fill: true,
          backgroundColor: gradient,
          borderColor: 'rgb(155, 155, 132)',
          pointBackgroundColor: 'rgb(25,25,112)',
          segment: {
            borderDash: ctx => skipped(ctx, [6, 6]),
          },
          spanGaps: true,
          data: temperatureData,
        }]
      };
      var config = {
        type: 'line',
        data: data,
        options: {
          indexAxis: 'x',
          radius: 3,
          hitRadius: 3,
          hoverRadius: 4,
          responsive: true,
          scales: {
            y: {
              ticks: {
                callback: function (value) { return value + '\u2103'; }
              }
            },
            x: {
              type: 'time',
              distribution: 'series',
              time: {
                displayFormats: {
                  hour: 'D/M h A'
                }
              },
              ticks: {
                source: 'auto',
                // Disabled rotation for performance
                maxRotation: 0,
                //autoSkip: true,
              }
            }
          }
        }
      };
      myChart = new Chart(ctx, config);
    }
    function init() {
      Socket = new WebSocket('ws://' + window.location.hostname + '/ws');
      Socket.onmessage = function (event) {
        var isJason = false;
        try {
          jsonObj = JSON.parse(event.data);
          isJason = true;
        }
        catch (err) {
          console.log("event data isn't JSON");
        }
        if (isJason) {
          if (jsonObj.type == 'sensor') {
            let temp = parseFloat(jsonObj.temperature);
            document.getElementById("temp").innerHTML = temp;
            document.getElementById("humi").innerHTML = parseFloat(jsonObj.humidity);
            let date = new Date;
            addData(myChart, date, temp);
          }
        }
        else { document.getElementById("rxConsole").value += event.data; }
      }
      createChart();
    }
    function addData(chart, label, data) {
      temperatureData.push({ x: label, y: data });
      chart.update();
    }
    function sendText() {
      Socket.send(document.getElementById("txBar").value);
      document.getElementById("txBar").value = "";
    }
    function sendfanpwm(val) {
      document.getElementById('rangevalue').textContent = `PWM Value: ${1024 - val}`;
      Socket.send("#" + document.getElementById("fanpwm").value);
    }
    function xml_handler() {
      console.log("button was clicked!");
      var xhr = new XMLHttpRequest();
      var url = "/xml";
      xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          var xmlDoc = this.responseXML;
          var x = xmlDoc.getElementsByTagName("status");
          y = x[0].getElementsByTagName("mode")[0].childNodes[0].nodeValue
          z = x[0].getElementsByTagName("ip")[0].childNodes[0].nodeValue
          document.getElementById("Status").innerHTML = y;
          document.getElementById("Ip").innerHTML = z;
        }
      };
      xhr.open("GET", url, true);
      xhr.send();
    };
    document.addEventListener('DOMContentLoaded', xml_handler, false);
  </script>
</head>

<body onload="javascript:init()">
  <main class="container">
    <div id="properties">
      <h2 style="color: #2e6c80;">Status:</h2>
      <p> Cached page loaded from flash memory.</p>
      <p> Connection Status: <span id="Status">__</span> </p>
      <p> Ip Address: <span id="Ip">__</span> </p>
      <button class="btn" onclick="xml_handler()"> Refresh </button>
    </div>
    <div class="chart">
      <h2 style="color: #2e6c80;">Sensors</h2>
      <canvas id="myChart"></canvas>
    </div>
    <div>
      <button class="btn" type="button" onclick="setMinLastHour(myChart)">Last Hour</button>
      <button class="btn" type="button" onclick="setMinLast24Hours(myChart)">Last 24 Hours</button>
      <button class="btn" type="button" onclick="resetMin(myChart)">All data</button>
    </div>
    <div class="sensor-output">
      <p> Temperature: <span id="temp">__</span>&#8451</p>
      <p> Humidity: <span id="humi">__</span>&#37</p>
    </div>
    <div>
      <h2 style="color: #2e6c80;">Serial Console:</h2>
      <textarea id="rxConsole" rows="5"></textarea>
    </div>
    <div>
      <input type="text" id="txBar" onkeydown="if(event.keyCode == 13) sendText();" />
      <button class="btn" type="button" onclick="sendText()">Send</button>
    </div>
    <div>
      <h2 style="color: #2e6c80;">PWM Control:</h2>
      <div class="fan-control">
        <input type="range" min="0" max="1024" id="fanpwm" value="364" oninput="sendfanpwm(this.value)" />
        <span id="rangevalue">PWM Value: 364</span>
      </div>
    </div>
    <div>
      <h2 style="color: #2e6c80;">Get Test:</h2>
      <form action="/get">
        <input type="text" name="input1">
        <input type="submit" class="btn" value="Submit">
      </form>
    </div>
    <div>
      <h2 style="color: #2e6c80;">File Upload :</h2>
      <form action='/edit' class="upload-form" method='post' enctype='multipart/form-data'>
        <input class='' type='file' name='fupload' id='fupload' value=''>
        <button class='btn upload-btn' type='submit'>Upload File</button>
      </form>
    </div>

  </main>
</body>
<style>
  @import url('https://fonts.googleapis.com/css?family=Poppins:400');

  *,
  *::before,
  *::after {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
  }

  img {
    max-width: 100%;
    object-fit: cover;
    display: block;
  }

  html {
    font-size: 100%;
  }

  body {
    background: #fefefe;
    font-family: 'Poppins', sans-serif;
    font-weight: 400;
    line-height: 1.75;
    color: #555;
  }

  p {
    margin-bottom: 1rem;
  }

  h1,
  h2,
  h3,
  h4,
  h5 {
    margin: 3rem 0 1.38rem;
    font-family: 'Poppins', sans-serif;
    font-weight: 400;
    line-height: 1.3;
  }

  h1 {
    margin-top: 0;
    font-size: 3.052rem;
  }

  h2 {
    font-size: 2.441rem;
  }

  h3 {
    font-size: 1.953rem;
  }

  h4 {
    font-size: 1.563rem;
  }

  h5 {
    font-size: 1.25rem;
  }

  small,
  .text_small {
    font-size: 0.8rem;
  }

  input,
  textarea {
    width: 100%;
    font-family: inherit;
    padding: 0.5rem;
    font-size: 1.2rem;
    margin-bottom: 1rem;
    display: block;
    max-width: 800px;
  }

  textarea {
    resize: none;
  }

  .container {
    width: 90%;
    max-width: 1200px;
    margin: 0 auto;
  }

  .chart {
    min-width: 300px;
    max-width: 100%;
    margin-bottom: 0.5rem;
  }

  .btn {
    display: inline-block;
    padding: 0.5rem 1.5rem;
    border: 1px solid #555;
    border-radius: 10px;
    font-size: 1rem;
    background: transparent;
    transition-duration: 300ms;
    width: 150px;
    margin: 0 auto;
  }

  .btn:hover {
    background-color: #555;
    color: #fff;
  }

  .fan-control span,
  .fan-control input {
    display: block;
  }

  #rangevalue {
    width: 150px;
  }

  .fan-control {
    display: grid;
    grid-template-columns: 2fr 1fr;
    gap: 2rem;
  }

  .upload-form input {
    margin-bottom: 0;
  }

  .sensor-output {
    display: flex;
    gap: 3rem;
    justify-content: center;
  }

  .upload-btn {
    margin-bottom: 2rem;
  }

  @media screen and (min-width: 500px) {
    .status {
      display: grid;
      grid-template-columns: 1fr 1fr;
    }

    .fan-control {
      max-width: 800px;
      grid-template-columns: 4fr 1fr;
      gap: 50px;
    }
  }
</style>

</html>
)=====";