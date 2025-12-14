const RequestApi = (() => {
  function postText(path, bodyText = "") {
    const url = new URL(path, window.location.href);
    return fetch(url, { method: 'POST', body: bodyText });
  }

  async function getVersion() {
    const resp = await postText('/getVersion');
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }

  async function getAutoThirdAppToggle() {
    const resp = await postText('/getAutoThirdAppToggle');
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }

  async function setAutoThirdAppToggle(content) {
    const resp = await postText('/setAutoThirdAppToggle', content);
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }

  async function getFixTeeToggle() {
    const resp = await postText('/getFixTeeToggle');
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }

  async function setFixTeeToggle(content) {
    const resp = await postText('/setFixTeeToggle', content);
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }
  
  async function getTargetTxt() {
    const resp = await postText('/getTargetTxt');
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }

  async function setTargetTxt(content) {
    const resp = await postText('/setTargetTxt', content);
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }

  async function getKeyboxXml() {
    const resp = await postText('/getKeyboxXml');
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }

  async function setKeyboxXml(content) {
    const resp = await postText('/setKeyboxXml', content);
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }


  return {
    getVersion,
    getAutoThirdAppToggle,
    setAutoThirdAppToggle,
    getFixTeeToggle,
    setFixTeeToggle,
    getTargetTxt,
    setTargetTxt,
    getKeyboxXml,
    setKeyboxXml,
  };
})();
