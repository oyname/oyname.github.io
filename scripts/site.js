function initSlider() {
    const slides = document.querySelectorAll('.slide');
    const indicators = document.querySelectorAll('.indicator');

    if (!slides.length || !indicators.length) return;

    let currentSlide = 0;
    let slideInterval;
    const slideCount = slides.length;

    function showSlide(index) {
        slides[currentSlide].classList.remove('active');
        indicators[currentSlide].classList.remove('active');

        currentSlide = index;

        slides[currentSlide].classList.add('active');
        indicators[currentSlide].classList.add('active');
    }

    function nextSlide() {
        showSlide((currentSlide + 1) % slideCount);
    }

    indicators.forEach((indicator, index) => {
        indicator.addEventListener('click', () => {
            showSlide(index);
            resetInterval();
        });
    });

    function resetInterval() {
        clearInterval(slideInterval);
        slideInterval = setInterval(nextSlide, 5000);
    }

    resetInterval();
}

function parseFrontmatter(text) {
    const frontmatterRegex = /^---\s*([\s\S]*?)\s*---\s*([\s\S]*)$/;
    const match = text.match(frontmatterRegex);

    if (!match) {
        return {
            meta: {},
            content: text
        };
    }

    const rawMeta = match[1];
    const content = match[2];
    const meta = {};

    rawMeta.split('\n').forEach(line => {
        const separatorIndex = line.indexOf(':');
        if (separatorIndex === -1) return;

        const key = line.slice(0, separatorIndex).trim();
        const value = line.slice(separatorIndex + 1).trim();
        meta[key] = value;
    });

    return { meta, content };
}

function getExampleFileFromQuery() {
    const params = new URLSearchParams(window.location.search);
    return params.get('file') || 'examples/example1.md';
}

function loadExampleFromMarkdown() {
    const markdownFile = getExampleFileFromQuery();

    fetch(markdownFile)
        .then(response => {
            if (!response.ok) throw new Error('Datei nicht gefunden');
            return response.text();
        })
        .then(text => {
            const { meta, content } = parseFrontmatter(text);

            const converter = new showdown.Converter({
                tables: true,
                ghCompatibleHeaderId: true,
                simplifiedAutoLink: true,
                strikethrough: true
            });

            document.getElementById('example-title').textContent = meta.title || 'Ohne Titel';
            document.getElementById('example-image').src = meta.image || '';
            document.getElementById('example-image').alt = meta.title || 'Beispielbild';
            document.getElementById('example-download').href = meta.download || '#';
            document.getElementById('example-download').textContent = meta.downloadLabel || 'Datei herunterladen';
            document.getElementById('example-description').innerHTML = converter.makeHtml(content);
        })
        .catch(error => {
            document.getElementById('example-description').innerHTML =
                '<div class="error">❌ Fehler beim Laden: ' + error.message + '</div>';
        });
}
