let translations = {}; // Stores loaded translations
let isLoaded = false;  // Flag to indicate when translations are ready

// Load translations immediately but do nothing to DOM yet
fetch('./Libs/translations.json')
  .then(res => res.json())
  .then(data => {
    translations = data;
    isLoaded = true;
    console.log("✅ Translations loaded");
  })
  .catch(error => {
    console.error("❌ Failed to load translations.json:", error);
  });

// Public function to call when ready to apply a language
function setLanguage(lang) {
  if (!isLoaded) {
    console.warn("⚠️ Translations not loaded yet.");
    return;
  }

  const selectedTranslations = translations[lang];
  if (!selectedTranslations) {
    console.error(`❌ No translations found for language: ${lang}`);
    return;
  }

  document.querySelectorAll('[data-i18n]').forEach(el => {
    const key = el.getAttribute('data-i18n');
    const value = selectedTranslations[key];
    if (value) {
      el.textContent = value;
    } else {
      console.warn(`⚠️ Missing key "${key}" in language "${lang}"`);
    }
  });
}
