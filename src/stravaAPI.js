var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

var persistentkey = 0;
var accessToken = localStorage.getItem(persistentkey);

Pebble.addEventListener("showConfiguration",
  function(e) {
    //Load the remote config page
    Pebble.openURL("http://crin.co.uk/stravaWatchface/config.php");
    console.log("Opened config page...");
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    //Get JSON dictionary
    console.log('Pebble Account Token: ' + Pebble.getAccountToken());
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log("Configuration window returned: " + configuration.accessToken);
    accessToken = configuration.accessToken;
    localStorage.setItem(persistentkey, configuration.accessToken); //Store the latest config data in local storage
    //Send to Pebble
    Pebble.sendAppMessage(
      {"KEY_ATHLETE": configuration.accessToken},
      function(e) {
        console.log("Sending settings data...");
							 
      },
      function(e) {
        console.log("Settings feedback failed!");
      }
    );
    getStats();
  }
);

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
    getStats();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getStats();
  }                     
);

function getStats() {
  // Construct URL
  var url = "http://crin.co.uk/stravaWatchface/getStats.php?a=" + accessToken;
  console.log(url);
  // Send request to crin.co.uk
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with stat info
      var json = JSON.parse(responseText);

      // Monthly miles
      var monthMiles = json.mM;
      console.log("Monthly total miles is: " + monthMiles);

      // Monthly Elevation
      var monthElevation = json.mE;      
      console.log("Monthly total elevation gain is: " + monthElevation);

      // Total Miles
      var totalMiles = json.tM;      
      console.log("All-time total miles is: " + totalMiles);

      // Total Elevation
      var totalElevation = json.tE;      
      console.log("All-time total elevation gain is: " + totalElevation);
      
      // Assemble dictionary using our keys
      var dictionary = {
       "KEY_MONTHMILES": monthMiles,
       "KEY_MONTHELEVATION": monthElevation,
       "KEY_TOTALMILES": totalMiles,
							"KEY_TOTALELEVATION": totalElevation
						};

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Stats info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending stats info to Pebble!");
        }
      );
    }      
  );
}