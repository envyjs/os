/* -*- Mode: JavaScript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
(function() {
  const windowTitle = 'Welcome to Envy';
  const customHTML = `
  
  <center>
   <div class="nav-buttons">
    <button class="buttonnav" onclick="prevPage()"><img src="./Assets/misc/back.svg"></button>
    <button class="buttonnav" onclick="nextPage()"><img src="./Assets/misc/front.svg"></button>
  </div>
  </center>
  <div class="centerpls">
  <div class="page active" id="page1">
    <h2>Welcome!</h2>
    <p>You're on the latest version of Envy, 2027 Update.</p>
  </div>

  <div class="page" id="page2">
    <h2>Page 2</h2>
    <p>This is the second page. You can add more content here.</p>
  </div>

  <div class="page" id="page3">
    <h2>You're all set!</h2>
    <p>You're on the last page. Thanks for visiting!</p>
  </div>
  </div>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 700, 450);
})();

let currentPage = 0;
    const pages = document.querySelectorAll('.page');

    function showPage(index) {
      pages.forEach((page, i) => {
        page.classList.toggle('active', i === index);
      });
    }

    function nextPage() {
      if (currentPage < pages.length - 1) {
        currentPage++;
        showPage(currentPage);
      }
    }

    function prevPage() {
      if (currentPage > 0) {
        currentPage--;
        showPage(currentPage);
      }
    }