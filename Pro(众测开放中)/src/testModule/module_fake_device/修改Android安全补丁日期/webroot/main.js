const patchDateInput = document.getElementById('patchDateInput');
const currentPatchDate = document.getElementById('currentPatchDate');
const saveBtn = document.getElementById('saveBtn');
const rebootMask = document.getElementById('rebootMask');
const rebootOkBtn = document.getElementById('rebootOkBtn');

function pad2(n) {
  return String(n).padStart(2, '0');
}

function isLeapYear(year) {
  return (year % 4 === 0 && year % 100 !== 0) || (year % 400 === 0);
}

function daysInMonth(year, month) {
  const days = [31, isLeapYear(year) ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
  return days[month - 1] || 0;
}

function normalizeSecurityPatchDate(value) {
  const text = String(value || '').trim();
  const m = /^(\d{4})-(\d{1,2})-(\d{1,2})$/.exec(text);
  if (!m) return '';

  const yearText = m[1];
  const year = Number(yearText);
  const month = Number(m[2]);
  const day = Number(m[3]);

  if (!Number.isInteger(year) || !Number.isInteger(month) || !Number.isInteger(day)) return '';
  if (month < 1 || month > 12) return '';
  if (day < 1 || day > daysInMonth(year, month)) return '';

  return yearText + '-' + pad2(month) + '-' + pad2(day);
}

function normalizeInputIfPossible() {
  const normalized = normalizeSecurityPatchDate(patchDateInput.value);
  if (normalized) {
    patchDateInput.value = normalized;
  }
}

function showRebootNotice() {
  if (!rebootMask) return;

  rebootMask.classList.add('show');
  rebootMask.setAttribute('aria-hidden', 'false');
  document.body.classList.add('modal-open');
}

function hideRebootNotice() {
  if (!rebootMask) return;

  rebootMask.classList.remove('show');
  rebootMask.setAttribute('aria-hidden', 'true');
  document.body.classList.remove('modal-open');
}

async function handleSave() {
  const normalized = normalizeSecurityPatchDate(patchDateInput.value);
  if (!normalized) {
    showToast('日期格式错误', 'error');
    return;
  }

  patchDateInput.value = normalized;
  saveBtn.disabled = true;
  saveBtn.textContent = '保存中...';

  try {
    const resp = await RequestApi.setSecurityPatchDate(normalized);
    if (resp !== 'OK') throw new Error(resp);

    if (currentPatchDate) currentPatchDate.textContent = normalized;
    showToast('修改成功');
    window.setTimeout(showRebootNotice, 1500);
  } catch (err) {
    console.error(err);
    showToast('保存失败', 'error');
  } finally {
    saveBtn.disabled = false;
    saveBtn.textContent = '保存配置';
  }
}

async function initData() {
  try {
    const currentRaw = await RequestApi.getSecurityPatchDate();
    const normalized = normalizeSecurityPatchDate(currentRaw);
    const displayValue = normalized || String(currentRaw || '').trim();

    if (currentPatchDate) currentPatchDate.textContent = displayValue || '未获取到';
    if (patchDateInput) patchDateInput.value = displayValue || '';
  } catch (err) {
    console.error(err);
    if (currentPatchDate) currentPatchDate.textContent = '读取失败';
    showToast('初始化失败', 'error');
  }
}

patchDateInput.addEventListener('blur', normalizeInputIfPossible);
patchDateInput.addEventListener('keydown', function (e) {
  if (e.key === 'Enter') {
    handleSave();
  }
});

saveBtn.addEventListener('click', handleSave);

if (rebootOkBtn) {
  rebootOkBtn.addEventListener('click', hideRebootNotice);
}

document.addEventListener('keydown', function (e) {
  if (e.key === 'Escape') {
    hideRebootNotice();
  }
});

initData();
