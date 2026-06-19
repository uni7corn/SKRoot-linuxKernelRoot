let toastTimer = null;
const toast = document.getElementById('toast');

function escapeToastText(text) {
  return String(text)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

function showToast(text, type = 'success') {
  const safeText = escapeToastText(text);
  const isError = type === 'error';

  toast.classList.remove('toast-success', 'toast-error');
  toast.classList.add(isError ? 'toast-error' : 'toast-success');

  toast.querySelector('.toast-inner').innerHTML = '' +
    '<div style="display:flex; flex-direction:column; align-items:center; justify-content:center; gap:8px;">' +
      '<div class="toast-icon">' +
        (isError
          ? '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" aria-hidden="true">' +
              '<path d="M8 8L16 16" stroke="#D92D20" stroke-width="2.4" stroke-linecap="round"/>' +
              '<path d="M16 8L8 16" stroke="#D92D20" stroke-width="2.4" stroke-linecap="round"/>' +
            '</svg>'
          : '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" aria-hidden="true">' +
              '<path d="M7 12.5L10.2 15.7L17.2 8.8" stroke="#28B44B" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round"/>' +
            '</svg>') +
      '</div>' +
      '<div class="toast-text">' + safeText + '</div>' +
    '</div>';

  toast.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => {
    toast.classList.remove('show');
  }, 2500);
}

function showErrorToast(text) {
  showToast(text, 'error');
}
