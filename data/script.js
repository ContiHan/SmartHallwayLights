"use strict";

document.addEventListener("DOMContentLoaded", function () {
  initializeModal();
  initializeSwitch();
});

function initializeModal() {
  const modal = document.querySelector(".modal");
  const overlay = document.querySelector(".overlay");
  const btnCloseModal = document.querySelector(".close-modal");
  const btnOpenModal = document.querySelector(".show-modal");

  const toggleModalVisibility = () => modal.classList.toggle("hidden");
  const toggleOverlayVisibility = () => overlay.classList.toggle("hidden");

  btnOpenModal.addEventListener("click", () => {
    toggleModalVisibility();
    toggleOverlayVisibility();
    updateModalData();
  });

  btnCloseModal.addEventListener("click", () => {
    toggleModalVisibility();
    toggleOverlayVisibility();
  });

  overlay.addEventListener("click", () => {
    toggleModalVisibility();
    toggleOverlayVisibility();
  });

  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape" && !modal.classList.contains("hidden")) {
      toggleModalVisibility();
      toggleOverlayVisibility();
    }
  });
}

async function fetchAndUpdate(url, updateFunction) {
  try {
    const response = await fetch(url);
    const data = await response.text();
    updateFunction(data);
  } catch (error) {
    console.error(`Error fetching ${url}:`, error);
  }
}

function initializeSwitch() {
  const checkbox = document.querySelector(".toggle-switch");
  const sectionMain = document.querySelector(".section-main");
  const sliderText = document.querySelector(".slider-text");
  const bulbIconOff = document.querySelector(".bulb-icon-off");
  const bulbIconOn = document.querySelector(".bulb-icon-on");

  const updateSwitchState = (isOn) => {
    checkbox.checked = isOn;
    sliderText.textContent = isOn ? "ON" : "OFF";
    sectionMain.classList.toggle("on", isOn);
    sectionMain.classList.toggle("off", !isOn);
    bulbIconOff.classList.toggle("hidden", isOn);
    bulbIconOn.classList.toggle("hidden", !isOn);
  };

  fetchAndUpdate("/current-led-state", (state) =>
    updateSwitchState(state === "on")
  );

  checkbox.addEventListener("change", () => {
    setTimeout(() => {
      const isOn = checkbox.checked;
      updateSwitchState(isOn);
      fetchAndUpdate(isOn ? "/led-on" : "/led-off", (data) =>
        console.log("Success:", data)
      );
    }, 400);
  });
}

function loadElapsedTime() {
  fetchAndUpdate("/time-since-startup", (data) => {
    document.getElementById(
      "time-since-startup"
    ).innerHTML = `Čas od spuštění je <strong>${data}</strong>`;
  });
}

function getPwmValue() {
  fetchAndUpdate("/pwm-value", (data) => {
    document.getElementById(
      "pwm-value"
    ).innerHTML = `PWM nastaveno na <strong>${data}&nbsp;%</strong>`;
  });
}

function updateModalData() {
  loadElapsedTime();
  getPwmValue();
}
