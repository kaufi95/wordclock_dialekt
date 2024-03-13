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
  mappedColor = colorMapper(color);
  colorBoxes.forEach((box) => {
    if (box.getAttribute('value') === mappedColor) {
      box.setAttribute('selected', true);
    } else {
      box.removeAttribute('selected');
    }
  });
}

function updateLanguage(language) {
  mappedLanguage = languageMapper(language);
  languageRadioButtons.forEach((button) => {
    if (button.value === mappedLanguage) {
      button.checked = true;
    }
  });
}

function updateBrightness(brightness) {
  mappedBrightness = brightnessMapper(brightness);
  brightnessRadioButtons.forEach((button) => {
    if (button.value === mappedBrightness) {
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
  fetch('http://192.168.4.1/status')
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

const colorMapper = (value) => {
  switch (value) {
    case '0':
      return 'white';
    case '1':
      return 'red';
    case '2':
      return 'green';
    case '3':
      return 'blue';
    case '4':
      return 'cyan';
    case '5':
      return 'magenta';
    case '6':
      return 'yellow';
    default:
      return 'white';
  }
};

const languageMapper = (value) => {
  switch (value) {
    case '0':
      return 'dialekt';
    case '1':
      return 'deutsch';
    default:
      return 'dialekt';
  }
};

const brightnessMapper = (value) => {
  switch (value) {
    case '64':
      return 'low';
    case '96':
      return 'mid';
    case '128':
      return 'high';
    case '160':
      return 'veryhigh';
    default:
      return 'mid';
  }
};
