// shared.js
const esp8266 = {
  "url": "",
  "cors": false
  // "url": "http://192.168.1.105",
  // "cors": true
};

function ajax_error(xhr, status, msg) {
  if (status === "timeout") {
    alert("Ajax timeout!");
  } else {
    var answer = JSON.parse(xhr.responseText);
    alert("" + answer.error.reason);
  }
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
}
