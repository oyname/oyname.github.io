console.log('site.js geladen');

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

function loadMarkdown(targetId, markdownFile) {
    console.log('loadMarkdown gestartet:', targetId, markdownFile);

    const target = document.getElementById(targetId);

    if (!target) {
        console.error('Target nicht gefunden:', targetId);
        return;
    }

    fetch(markdownFile)
        .then(response => {
            console.log('fetch response:', response.status, markdownFile);
            if (!response.ok) throw new Error('Datei nicht gefunden: ' + markdownFile);
            return response.text();
        })
        .then(markdown => {
            console.log('markdown geladen');

            if (typeof showdown === 'undefined') {
                throw new Error('showdown wurde nicht geladen');
            }

            const converter = new showdown.Converter({
                tables: true,
                ghCompatibleHeaderId: true,
                simplifiedAutoLink: true,
                strikethrough: true
            });

            target.innerHTML = converter.makeHtml(markdown);
        })
        .catch(error => {
            console.error('loadMarkdown Fehler:', error);
            target.innerHTML =
                '<div class="error">❌ Fehler beim Laden: ' + error.message + '</div>';
        });
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

function createExampleHtml(meta, htmlContent) {
    return `
        <section class="example-item">
            <div class="example-image">
                <img src="${meta.image || ''}" alt="${meta.title || 'Beispielbild'}">
            </div>
            <div class="example-text">
                <h2>${meta.title || 'Ohne Titel'}</h2>
                <div class="example-description">
                    ${htmlContent}
                </div>
                ${
                    meta.download
                        ? `<a class="download-button" href="${meta.download}" download>
                            ${meta.downloadLabel || 'Datei herunterladen'}
                           </a>`
                        : ''
                }
            </div>
        </section>
    `;
}

async function loadAllExamples() {
    const container = document.getElementById('examples-container');

    try {
        const listResponse = await fetch('data/examples.json');
        if (!listResponse.ok) {
            throw new Error('examples.json konnte nicht geladen werden');
        }

        const examples = await listResponse.json();

        if (!Array.isArray(examples) || examples.length === 0) {
            container.innerHTML = '<div class="error">❌ Keine Beispiele gefunden.</div>';
            return;
        }

        const converter = new showdown.Converter({
            tables: true,
            ghCompatibleHeaderId: true,
            simplifiedAutoLink: true,
            strikethrough: true
        });

        const renderedExamples = [];

        for (const item of examples) {
            try {
                const response = await fetch(item.file);
                if (!response.ok) {
                    renderedExamples.push(`
                        <div class="error">
                            ❌ Datei nicht gefunden: ${item.file}
                        </div>
                    `);
                    continue;
                }

                const text = await response.text();
                const { meta, content } = parseFrontmatter(text);
                const htmlContent = converter.makeHtml(content);

                renderedExamples.push(createExampleHtml(meta, htmlContent));
            } catch (err) {
                renderedExamples.push(`
                    <div class="error">
                        ❌ Fehler bei ${item.file}: ${err.message}
                    </div>
                `);
            }
        }

        container.innerHTML = renderedExamples.join('');
    } catch (error) {
        container.innerHTML = `<div class="error">❌ Fehler beim Laden: ${error.message}</div>`;
    }
}
