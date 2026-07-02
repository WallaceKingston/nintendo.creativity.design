const catalog = [
  { title: 'Mushroom Kingdom Movie Club', type: 'movie', description: 'A cheerful feature-length comedy about teamwork, timing, and impossible jumps.', accent: '#e60012' },
  { title: 'Hyrule After Dark', type: 'show', description: 'A cinematic anthology of legends, puzzles, and campfire stories.', accent: '#24b36b' },
  { title: 'Kart Circuit: Turbo Tales', type: 'show', description: 'Racers chase trophies and friendships in a fast-paced animated series.', accent: '#00a3ff' },
  { title: 'Direct Premiere Festival', type: 'special', description: 'A celebration of trailers, creator chats, and surprise short films.', accent: '#7d5cff' },
  { title: 'Dreamland Dessert Quest', type: 'movie', description: 'A cozy adventure filled with floating islands, music, and snacks.', accent: '#ff79c6' },
  { title: 'Behind the Cartridge', type: 'special', description: 'Artists and composers reveal how playful worlds become screen stories.', accent: '#ff9f1c' }
];

const grid = document.querySelector('#catalog-grid');
const filters = document.querySelectorAll('.filter');
const modal = document.querySelector('#trailer-modal');
const modalTitle = document.querySelector('#modal-title');
const navToggle = document.querySelector('.nav-toggle');
const navLinks = document.querySelector('#nav-links');

function renderCatalog(filter = 'all') {
  grid.innerHTML = '';
  catalog
    .filter(item => filter === 'all' || item.type === filter)
    .forEach(item => {
      const card = document.createElement('article');
      card.className = 'catalog-card';
      card.style.setProperty('--accent', item.accent);
      card.innerHTML = `
        <small>${item.type}</small>
        <h3>${item.title}</h3>
        <p>${item.description}</p>
        <button class="play-button" data-title="${item.title}">▶ Watch trailer</button>
      `;
      grid.appendChild(card);
    });
}

filters.forEach(button => {
  button.addEventListener('click', () => {
    filters.forEach(filter => filter.classList.remove('active'));
    button.classList.add('active');
    renderCatalog(button.dataset.filter);
  });
});

document.addEventListener('click', event => {
  const playButton = event.target.closest('.play-button');
  if (!playButton) return;
  modalTitle.textContent = playButton.dataset.title;
  modal.classList.add('open');
  modal.setAttribute('aria-hidden', 'false');
});

document.querySelector('.modal-close').addEventListener('click', closeModal);
modal.addEventListener('click', event => {
  if (event.target === modal) closeModal();
});

document.addEventListener('keydown', event => {
  if (event.key === 'Escape') closeModal();
});

navToggle.addEventListener('click', () => {
  const isOpen = navLinks.classList.toggle('open');
  navToggle.setAttribute('aria-expanded', String(isOpen));
});

function closeModal() {
  modal.classList.remove('open');
  modal.setAttribute('aria-hidden', 'true');
}

renderCatalog();
