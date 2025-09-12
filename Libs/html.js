export default class Html {
  elm;
  constructor(e) {
    this.elm = document.createElement(e || "div");
  }
  text(val) {
    this.elm.innerText = val;
    return this;
  }
  html(val) {
    this.elm.innerHTML = val;
    return this;
  }
  cleanup() {
    this.elm.remove();
  }
  query(selector) {
    return this.elm.querySelector(selector);
  }
  class(...val) {
    for (let i = 0; i < val.length; i++) {
      this.elm.classList.toggle(val[i]);
    }
    return this;
  }
  classOn(...val) {
    for (let i = 0; i < val.length; i++) {
      this.elm.classList.add(val[i]);
    }
    return this;
  }
  classOff(...val) {
    for (let i = 0; i < val.length; i++) {
      this.elm.classList.remove(val[i]);
    }
    return this;
  }
  style(obj) {
    for (const key of Object.keys(obj)) {
      this.elm.style.setProperty(key, obj[key]);
    }
    return this;
  }
  on(ev, cb) {
    this.elm.addEventListener(ev, cb);
    return this;
  }
  un(ev, cb) {
    this.elm.removeEventListener(ev, cb);
    return this;
  }
  appendTo(parent) {
    if (parent instanceof HTMLElement) {
      parent.appendChild(this.elm);
    } else if (parent instanceof Html) {
      parent.elm.appendChild(this.elm);
    } else if (typeof parent === "string") {
      document.querySelector(parent)?.appendChild(this.elm);
    }
    return this;
  }
  append(elem) {
    if (elem instanceof HTMLElement) {
      this.elm.appendChild(elem);
    } else if (elem instanceof Html) {
      this.elm.appendChild(elem.elm);
    } else if (typeof elem === "string") {
      const newElem = document.createElement(elem);
      this.elm.appendChild(newElem);
      return new Html(newElem.tagName);
    }
    return this;
  }
  appendMany(...elements) {
    for (const elem of elements) {
      this.append(elem);
    }
    return this;
  }
  clear() {
    this.elm.innerHTML = "";
    return this;
  }
  attr(obj) {
    for (let key in obj) {
      if (obj[key] !== null) this.elm.setAttribute(key, obj[key]);
      else this.elm.removeAttribute(key);
    }
    return this;
  }
  val(str) {
    var x = this.elm;
    x.value = str;
  }
}