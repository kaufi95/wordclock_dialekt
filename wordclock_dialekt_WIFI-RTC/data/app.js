// document.addEventListener('DOMContentLoaded', function () {
// Function to send HTTP POST request
function sendPostRequest(color) {
  const language = getSelectedLanguage();
  const brightness = getSelectedBrightness();

  if (language === '') {
    alert('Please select a language');
    return;
  }

  if (brightness === '') {
    alert('Please select a brightness');
    return;
  }

  const body = {
    datetime: new Date().valueOf().toString().slice(0, -3),
    color: color,
    language: language,
    brightness: brightness
  };

  fetch('http://192.168.4.1/update', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(body)
  })
    .then((response) => {
      if (!response.ok) {
        throw new Error('Network response was not ok');
      }
      console.log('POST request sent successfully');
    })
    .catch((error) => console.error('Error sending POST request:', error));
}

const colorBoxes = document.querySelectorAll('.color-box');
colorBoxes.forEach((box) => {
  box.addEventListener('click', function () {
    sendPostRequest(this.style.backgroundColor);
  });
});

function getSelectedLanguage() {
  let selectedLanguage = '';
  const languageRadioButtons = document.querySelectorAll(
    'input[name="language"]'
  );
  languageRadioButtons.forEach((button) => {
    if (button.checked) {
      selectedLanguage = button.value;
    }
  });
  return selectedLanguage;
}

function getSelectedBrightness() {
  let selectedBrightness = '';
  const brightnessRadioButtons = document.querySelectorAll(
    'input[name="brightness"]'
  );
  brightnessRadioButtons.forEach((button) => {
    if (button.checked) {
      selectedBrightness = button.value;
    }
  });
  return selectedBrightness;
}
// });
