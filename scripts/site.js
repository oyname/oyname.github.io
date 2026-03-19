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
    fetch(markdownFile)
        .then(response => {
            if (!response.ok) throw new Error('Datei nicht gefunden');
            return response.text();
        })
        .then(markdown => {
            const converter = new showdown.Converter({
                tables: true,
                ghCompatibleHeaderId: true,
                simplifiedAutoLink: true,
                strikethrough: true
            });

            document.getElementById(targetId).innerHTML = converter.makeHtml(markdown);
        })
        .catch(error => {
            document.getElementById(targetId).innerHTML =
                '<div class="error">❌ Fehler beim Laden: ' + error.message + '</div>';
        });
}
