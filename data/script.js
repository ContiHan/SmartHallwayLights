function loadElapsedTime() {
  fetch("/time-since-startup")
    .then((response) => response.text())
    .then((data) => {
      document.getElementById(
        "time-since-startup"
      ).innerHTML = `Čas od spuštění je <strong>${data}</strong>`;
    });
}

function getPwmValue() {
  fetch("/pwm-value")
    .then((response) => response.text())
    .then((data) => {
      document.getElementById(
        "pwm-value"
      ).innerHTML = `PWM nastaveno na <strong>${data} %</strong>`;
    });
}

function loadAllDataOnRefresh() {
  loadElapsedTime();
  getPwmValue();
}

document.addEventListener("DOMContentLoaded", loadAllDataOnRefresh);
