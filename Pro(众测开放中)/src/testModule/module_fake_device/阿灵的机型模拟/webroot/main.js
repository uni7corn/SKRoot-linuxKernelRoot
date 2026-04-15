const listArea = document.getElementById('listArea');
const searchInput = document.getElementById('searchInput');
const resultCount = document.getElementById('resultCount');
const currentMain = document.getElementById('currentMain');
const currentCode = document.getElementById('currentCode');
const saveBtn = document.getElementById('saveBtn');

// 本地机型列表：name 用来显示，sh 用来读写配置
const models = [
  { name: "红魔10s Pro", sh: "hm10s_pro.sh" },
  { name: "红魔电竞平板3 Pro", sh: "hmpad3_pro.sh" },
  { name: "华为Mate80 ProMax", sh: "huawei_mate80_promax.sh" },
  { name: "华为Mate80 ProMax 风驰版", sh: "huawei_mate80_promax_fengchi.sh" },
  { name: "一加15", sh: "oneplus15.sh" },
  { name: "一加Ace6", sh: "oneplus_ace6.sh" },
  { name: "红米K90", sh: "redmi_k90.sh" },
  { name: "IQOO 15", sh: "iqoo_15.sh" },
  { name: "IQOO 15 Ultra", sh: "iqoo_15_ultra.sh" },
  { name: "Oppo FindX8Ultra", sh: "oppo_find_x8_ultra.sh" },
  { name: "摩托罗拉 X70 Air Pro", sh: "moto_x70_air_pro.sh" }
];

// selected 保存当前选中的 sh
let selected = "";

function escapeHtml(text) {
  return String(text)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

function getFilteredModels(keyword) {
  const k = keyword.trim().toLowerCase();
  if (!k) return models.slice();
  return models.filter(item => String(item.name || '').toLowerCase().includes(k));
}

function findModelBySh(sh) {
  const target = String(sh || '').trim();
  if (!target) return null;
  return models.find(item => item && item.sh === target) || null;
}

function findModelByName(name) {
  const target = String(name || '').trim();
  if (!target) return null;
  return models.find(item => item && item.name === target) || null;
}

function updateCurrentInfo() {
  if (!selected) {
    if (currentMain) currentMain.textContent = '未选择';
    if (currentCode) currentCode.textContent = '-';
    return;
  }

  const model = findModelBySh(selected);

  if (currentMain) currentMain.textContent = model ? model.name : '未选择';
  if (currentCode) currentCode.textContent = model ? model.sh : '-';
}

function updateResultCount(count) {
  if (resultCount) {
    resultCount.textContent = '共 ' + count + ' 项';
  }
}

function renderList() {
  const filtered = getFilteredModels(searchInput.value);
  updateResultCount(filtered.length);
  updateCurrentInfo();

  if (!filtered.length) {
    listArea.innerHTML = '<div class="empty">未找到匹配机型</div>';
    return;
  }

  let html = '<div class="model-list">';

  filtered.forEach(item => {
    const name = String(item.name || '').trim();
    const sh = String(item.sh || '').trim();
    if (!name || !sh) return;

    const active = (selected === sh);
    html += ''
      + '<button class="model-item' + (active ? ' active' : '') + '"'
      + ' data-sh="' + escapeHtml(sh) + '">'
      +   '<div class="item-row">'
      +     '<div class="model-name">' + escapeHtml(name) + '</div>'
      +     (active
              ? '<div style="flex-shrink:0; color:#007aff; font-size:17px; font-weight:700; line-height:1;">✓</div>'
              : '')
      +   '</div>'
      + '</button>';
  });

  html += '</div>';
  listArea.innerHTML = html;
}

function bindListEvents() {
  listArea.addEventListener('click', function (e) {
    const btn = e.target.closest('.model-item');
    if (!btn) return;

    const sh = btn.getAttribute('data-sh');
    if (!sh) return;
    if (!findModelBySh(sh)) return;

    selected = sh;
    renderList();
    scrollToSelected();
  });
}

function scrollToSelected() {
  const active = listArea.querySelector('.model-item.active');
  if (!active) return;
  active.scrollIntoView({ block: 'center', behavior: 'auto' });
}

async function handleSave() {
  if (!selected) {
    showToast('请先选择机型');
    return;
  }

  saveBtn.disabled = true;
  saveBtn.textContent = '保存中...';

  try {
    // 这里传 sh
    const resp = await RequestApi.setCurrentSh(selected);
    if (resp != "OK") throw new Error(resp);
    showToast('保存成功，重启生效');
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
    if (!models.length) {
      throw new Error('phone list is empty');
    }

    // 这里拿到的是 sh
    const currentRaw = await RequestApi.getCurrentSh();
    const currentSh = String(currentRaw || '').trim();
    selected = findModelBySh(currentSh) ? currentSh : '';
    renderList();
    scrollToSelected();
  } catch (err) {
    console.error(err);
    selected = '';
    renderList();
    showToast('初始化列表失败', 'error');
  }
}

searchInput.addEventListener('input', function () {
  renderList();
});

saveBtn.addEventListener('click', handleSave);

bindListEvents();
initData();