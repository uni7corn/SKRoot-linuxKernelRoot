const RequestApi = (() => {
  const GET_SECURITY_PATCH_DATE_PATH = '/getSecurityPatchDate';
  const SET_SECURITY_PATCH_DATE_PATH = '/setSecurityPatchDate';

  function postText(path, bodyText = '') {
    const url = new URL(path, window.location.href);
    return fetch(url, {
      method: 'POST',
      headers: {
        'Content-Type': 'text/plain; charset=utf-8'
      },
      body: bodyText
    });
  }

  async function getSecurityPatchDate() {
    const resp = await postText(GET_SECURITY_PATCH_DATE_PATH);
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }

  async function setSecurityPatchDate(dateText) {
    const resp = await postText(SET_SECURITY_PATCH_DATE_PATH, dateText);
    return resp.ok ? await resp.text() : ('HTTP ' + resp.status);
  }

  return {
    getSecurityPatchDate,
    setSecurityPatchDate
  };
})();
