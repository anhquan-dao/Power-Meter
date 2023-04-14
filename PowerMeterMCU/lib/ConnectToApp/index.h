const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}=</style>
</head>

<body>
    <h1>ESP32 Web Server</h1>
    <div>
        <h4>The ESP32 Update web page without refresh</h4>
        <br>
        <h1>Wifi status:<span id="wifiStatus">0</span></h1>
        <br>
        <h1>Host status:<span id="hostStatus">0</span></h1>
        <br>
    </div>

    <form action="/wifi_info", method="POST", id="wifi_info", name="server_info", accept-charset=utf-8>
        Wifi SSID: <input type="text" name="ssid" id="Wifi SSID">
        <br>
        Wifi Pass: <input type="password" name="pass" id="Wifi Pass">
        <br>
        <input type="submit" value="Submit">
        <br>
    </form>

    <form action="/server_info", method="POST", id="server_info", name="server_info", accept-charset=utf-8>
        Server IP: <input type="text" name="server_ip" id="Server IP">
        <br>
        <input type="submit" value="Submit">
        <br>
    </form>

    <button type="button" onclick="reset()">Click Me!</button>

    <script type="text/javascript">
        
        window.onload = function()
        {
            var form = document.querySelector("form");
            form.onsubmit = submitForm.bind(form);
        }

        function serialize (data) {
            let obj = {};
            for (let [key, value] of data) {
                if (obj[key] !== undefined) {
                    if (!Array.isArray(obj[key])) {
                        obj[key] = [obj[key]];
                    }
                    obj[key].push(value);
                } else {
                    obj[key] = value;
                }
            }
            return obj;
        }

        function submitForm(e)
        {   
            e.preventDefault()
            
            var formID = e.target.id;
            var form = document.getElementById(formID);
            var data = new FormData(form);

            // Convert to an object
            let formObj = serialize(data);
            let formString = new URLSearchParams(Object.entries(formObj)).toString();

            console.log(formString);

            var xhttp = new XMLHttpRequest();
            xhttp.open("POST", formID, true);

            xhttp.setRequestHeader('Accept', '*/*');
            xhttp.setRequestHeader('Accept-Encoding', 'Accept-Encoding: gzip, deflate');
            xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded; charset=UTF-8')
            
            console.log(xhttp);
            
            xhttp.send(formString);
            
            xhttp.onload = function () {
                if(xhttp.status === 201) 
                {
                    console.log("Post successfully created!");

                }
            }

            form.reset();
        }

        function getData() 
        {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() 
            {
                if (this.readyState == 4 && this.status == 200) 
                {
                    console.log(this.response);
                    var obj = JSON.parse(this.responseText);
                    console.log(obj);
                    document.getElementById("wifiStatus").innerHTML =
                    obj.connect;
                    document.getElementById("hostStatus").innerHTML =
                    obj.host_status;
                }
            };
            xhttp.open("GET", "wifiStatus", true);
            xhttp.send(null);
        }
        
        // Call a function repetatively with 2 Second interval
        setInterval(
            function(){getData();}, 5000 //2000mSeconds update rate
            );
            
        function reset()
        {
            var xhttp = new XMLHttpRequest();
            xhttp.open("GET", "reset", true);
            xhttp.send(null);
        }


    </script>


    <!--
    <script>

        

        
    </script>
    -->

</body>
</html>
)=====";

const char OK_resp[] PROGMEM = R"=====(
HTTP/1.1 200 OK
Content-type:text/html
Connection: close
)=====";

const char OK_resp_plane[] PROGMEM = R"=====(
HTTP/1.1 200 OK
Content-type:text/html
Connection: close
)=====";