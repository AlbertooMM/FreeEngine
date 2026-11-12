let listaGlobalFondos = [];

// Escuchar el evento DOMContentLoaded para asegurar que el DOM esté cargado
document.addEventListener('DOMContentLoaded', () => {
    cargarCatálogo();

    // Evento del buscador
    const buscador = document.getElementById('buscador');
    if (buscador) {
        buscador.addEventListener('input', filtrarFondos);
    }

    // Evento del control de volumen
    const volumeSlider = document.getElementById('globalVolume');
    if (volumeSlider) {
        volumeSlider.addEventListener('input', (e) => {
            const volumen = parseFloat(e.target.value);
            
            // Creamos un paquete para enviar el volumen a C++
            const payload = {
                type: "set_volume",
                volume: volumen
            };

            if (window.chrome && window.chrome.webview) {
                window.chrome.webview.postMessage(JSON.stringify(payload));
            } else {
                console.log(`[JS] Volumen cambiado a: ${volumen}`);
            }
        });
    }
});

function cargarCatálogo() {
    fetch('fondos.json')
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP Error: ${response.status}\n${response.statusText}`);
            }
            return response.json();
        })
        .then(fondos => {
            listaGlobalFondos = fondos;
            renderizarFondos(listaGlobalFondos);
        })
        .catch(error => {
            console.error("Error cargando fondos.json:", error);
            const contenedor = document.getElementById('contenedor-fondos');
            if (contenedor) {
                contenedor.innerHTML = 
                    `<p class="no-results" style="color: #f87171;">Error al cargar el catálogo: ${error.message}</p>`;
            }
        });
}

function renderizarFondos(fondos) {
    const contenedor = document.getElementById('contenedor-fondos');
    if (!contenedor) return;

    contenedor.innerHTML = '';

    if (fondos.length === 0) {
        contenedor.innerHTML = `<p class="no-results">No se han encontrado fondos compatibles.</p>`;
        return;
    }

    fondos.forEach(fondo => {
        const card = document.createElement('div');
        card.className = 'card';

        const imagenHtml = fondo.preview
            ? `<img src="${fondo.preview}" class="card-image" alt="${escapeHtml(fondo.titulo)}" onerror="this.style.display='none'">`
            : `<div class="card-image" style="display:flex; align-items:center; justify-content:center; color:#64748b; font-size:0.8rem;">Sin vista previa</div>`;

        card.innerHTML = `
            ${imagenHtml}
            <div class="card-content">
                <h3>${escapeHtml(fondo.titulo)}</h3>
            </div>
            <footer>
                <button class="btn-apply" onclick="aplicarFondo('${fondo.id}', '${escapeHtml(fondo.titulo)}')">Aplicar Fondo</button>
            </footer>
        `;
        contenedor.appendChild(card);
    });
}

function filtrarFondos() {
    const buscador = document.getElementById('buscador');
    if (!buscador) return;

    const texto = buscador.value.toLowerCase();
    const filtrados = listaGlobalFondos.filter(fondo =>
        (fondo.titulo && fondo.titulo.toLowerCase().includes(texto)) || 
        (fondo.id && fondo.id.toLowerCase().includes(texto))
    );
    renderizarFondos(filtrados);
}

function aplicarFondo(idFondo, tituloFondo) {
    const monitorSelect = document.getElementById('select-monitor');
    const monitorIndex = monitorSelect ? parseInt(monitorSelect.value) : 0;

    // Creamos un objeto con el fondo y el monitor elegido
    const payload = {
        type: "apply_wallpaper",
        id: idFondo,
        monitor: monitorIndex
    };

    if (window.chrome && window.chrome.webview) {
        // Enviamos el objeto convertido a cadena JSON
        window.chrome.webview.postMessage(JSON.stringify(payload));
        console.log(`[JS] Enviado a C++ -> Fondo: ${idFondo}, Monitor: ${monitorIndex}`);
    } else {
        console.warn("[JS] WebView2 postMessage no está disponible.");
    }
}

function escapeHtml(text) {
    if (!text) return '';
    return text
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}