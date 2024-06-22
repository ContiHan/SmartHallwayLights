document.addEventListener("DOMContentLoaded", function () {
  const checkbox = document.getElementById("toggle-switch");
  const sectionMain = document.querySelector(".section-main");
  const sliderText = document.querySelector(".slider-text");
  const bulbIconOff = document.querySelector(".bulb-icon-off");
  const bulbIconOn = document.querySelector(".bulb-icon-on");

  // Funkce pro aktualizaci stavu přepínače
  function updateSwitchState(isOn) {
    if (isOn) {
      checkbox.checked = true;
      sliderText.textContent = "ON";
      sectionMain.classList.remove("off");
      sectionMain.classList.add("on");
      bulbIconOff.classList.add("hidden");
      bulbIconOn.classList.remove("hidden");
    } else {
      checkbox.checked = false;
      sliderText.textContent = "OFF";
      sectionMain.classList.remove("on");
      sectionMain.classList.add("off");
      bulbIconOff.classList.remove("hidden");
      bulbIconOn.classList.add("hidden");
    }
  }

  // Načítání počátečního stavu ze serveru
  fetch("/current-led-state")
    .then((response) => response.text())
    .then((state) => {
      updateSwitchState(state === "on");
    })
    .catch((error) => {
      console.error("Error fetching current led state:", error);
    });

  checkbox.addEventListener("change", function () {
    setTimeout(function () {
      const isOn = checkbox.checked;
      updateSwitchState(isOn);

      // Odeslání požadavku na server
      fetch(isOn ? "/led-on" : "/led-off")
        .then((response) => response.text())
        .then((data) => {
          console.log("Success:", data);
        })
        .catch((error) => {
          console.error("Error:", error);
        });
    }, 400); // Delay the text change to match the slider transition
  });
});

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
