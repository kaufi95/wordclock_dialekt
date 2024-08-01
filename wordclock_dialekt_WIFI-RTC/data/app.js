const colorBoxes = document.querySelectorAll('.color-box');
const languageRadioButtons = document.querySelectorAll(
  'input[name="language"]'
);
const brightnessRadioButtons = document.querySelectorAll(
  'input[name="brightness"]'
);

document.addEventListener('DOMContentLoaded', () => {
  onLoad();
});

colorBoxes.forEach((box) => {
  box.addEventListener('click', () => {
    colorBoxes.forEach((box) => {
      box.removeAttribute('selected');
    });
    box.setAttribute('selected', true);
  });
});

function updateUI(data) {
  updateColor(data.color);
  updateLanguage(data.language);
  updateBrightness(data.brightness);
}

function updateColor(color) {
  colorBoxes.forEach((box) => {
    if (box.getAttribute('value') === color) {
      box.setAttribute('selected', true);
    } else {
      box.removeAttribute('selected');
    }
  });
}

function updateLanguage(language) {
  languageRadioButtons.forEach((button) => {
    if (button.value === language) {
      button.checked = true;
    }
  });
}

function updateBrightness(brightness) {
  brightnessRadioButtons.forEach((button) => {
    if (button.value === brightness) {
      button.checked = true;
    }
  });
}

function getSelectedColor() {
  let selectedColor = '';
  colorBoxes.forEach((box) => {
    if (box.hasAttribute('selected')) {
      selectedColor = box.getAttribute('value');
    }
  });
  return selectedColor;
}

function getSelectedLanguage() {
  let selectedLanguage = '';
  languageRadioButtons.forEach((button) => {
    if (button.checked) {
      selectedLanguage = button.value;
    }
  });
  return selectedLanguage;
}

function getSelectedBrightness() {
  let selectedBrightness = '';
  brightnessRadioButtons.forEach((button) => {
    if (button.checked) {
      selectedBrightness = button.value;
    }
  });
  return selectedBrightness;
}

function onLoad() {
  fetch('http://' + window.location.host + '/status')
    .then((response) => {
      if (!response.ok) {
        throw new Error('Network response was not ok');
      }
      return response.json();
    })
    .then((data) => {
      console.log('GET request successful');
      console.log(data);
      updateUI(data);
    })
    .catch((error) => console.error('Error sending GET request:', error));
}

function sendPostRequest() {
  const color = getSelectedColor();
  const language = getSelectedLanguage();
  const brightness = getSelectedBrightness();

  if (color === '') {
    alert('Please select a color');
    return;
  }

  if (language === '') {
    alert('Please select a language');
    return;
  }

  if (brightness === '') {
    alert('Please select a brightness');
    return;
  }

  const body = {
    color: color,
    language: language,
    brightness: brightness,
    datetime: new Date().valueOf().toString().slice(0, -3)
  };

  fetch('http://' + window.location.host + '/update', {
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
