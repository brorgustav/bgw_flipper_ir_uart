#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <furi_hal_serial.h>
#include <infrared.h>
#include <infrared_worker.h>
#include <toolbox/stream/file_stream.h>
#include <toolbox/stream/stream.h>
#include <storage/storage.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_BUF 64
#define HOLD_TIME_MS 1000

typedef struct {
    FuriHalSerialHandle* serial_handle;
    bool running;
    bool log_to_file;
    uint32_t last_rng;
    FuriMutex* mutex;
    bool in_menu;
    uint32_t back_pressed_time;
    uint32_t ok_pressed_time;
} FlameTunnelState;

/* Generate RNG from IR signal */
static uint32_t generate_rng(InfraredWorkerSignal* sig) {
    const uint32_t* times;
    size_t n;
    infrared_worker_get_raw_signal(sig, &times, &n);
    uint32_t seed = furi_hal_random_get() ^ DWT->CYCCNT;
    for(size_t i = 0; i < n; i++) seed ^= times[i] + i;
    return seed;
}

/* Handle an IR event */
static void process_ir(FlameTunnelState* s, uint32_t rng) {
    char buf[MAX_BUF];
    int len = snprintf(buf, MAX_BUF, "RNG:%lu\n", (unsigned long)rng);
    furi_hal_serial_tx(s->serial_handle, (const uint8_t*)buf, len);

    if(s->log_to_file) {
        Storage* storage = furi_record_open(RECORD_STORAGE);
        Stream* file = file_stream_alloc(storage);
        file_stream_open(file, "/ext/flame_tunnel.log", FSAM_WRITE, FSOM_OPEN_ALWAYS);
        stream_write(file, (const uint8_t*)buf, len);
        file_stream_close(file);
        furi_record_close(RECORD_STORAGE);
    }
    s->last_rng = rng;
}

/* IR worker callback */
static void ir_callback(void* ctx, InfraredWorkerSignal* sig) {
    FlameTunnelState* s = ctx;
    uint32_t rng = generate_rng(sig);
    furi_mutex_acquire(s->mutex, FuriWaitForever);
    process_ir(s, rng);
    furi_mutex_release(s->mutex);
}

/* Draw UI */
static void draw(Canvas* canvas, void* ctx) {
    FlameTunnelState* s = ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    if(s->in_menu) {
        canvas_draw_str(canvas, 2, 10, "Config Menu");
        canvas_draw_str(canvas, 2, 30, s->log_to_file ? "Log ON" : "Log OFF");
    } else {
        char buf[32];
        canvas_draw_str(canvas, 2, 10, "Flame Tunnel");
        snprintf(buf, sizeof(buf), "%06lu", (unsigned long)s->last_rng);
        canvas_set_font(canvas, FontBigNumbers);
        canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignCenter, buf);
    }
}

/* Input handler */
static void input_cb(InputEvent* ev, void* ctx) {
    FlameTunnelState* s = ctx;
    uint32_t now = furi_get_tick();

    if(ev->type == InputTypePress) {
        if(ev->key == InputKeyBack) s->back_pressed_time = now;
        if(ev->key == InputKeyOk) s->ok_pressed_time = now;
    } else if(ev->type == InputTypeRelease) {
        if(ev->key == InputKeyBack && now - s->back_pressed_time >= HOLD_TIME_MS) {
            s->running = false;
        } else if(ev->key == InputKeyOk && now - s->ok_pressed_time >= HOLD_TIME_MS) {
            s->in_menu = !s->in_menu;
        } else if(s->in_menu && ev->key == InputKeyOk) {
            s->log_to_file = !s->log_to_file;
        }
    }
}

/* Application entry point */
int32_t bgw_flipper_ir_serial_app(void* p) {
    UNUSED(p);
    FlameTunnelState st = {
        .running = true,
        .log_to_file = false,
        .last_rng = 0,
        .mutex = furi_mutex_alloc(FuriMutexTypeNormal),
        .in_menu = false,
        .back_pressed_time = 0,
        .ok_pressed_time = 0,
    };

    furi_hal_serial_control_init();
    st.serial_handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    furi_hal_serial_init(st.serial_handle, 115200);

    ViewPort* vp = view_port_alloc();
    view_port_draw_callback_set(vp, draw, &st);
    view_port_input_callback_set(vp, input_cb, &st);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, vp, GuiLayerFullscreen);

    InfraredWorker* worker = infrared_worker_alloc();
    infrared_worker_rx_set_received_signal_callback(worker, ir_callback, &st);
    infrared_worker_rx_start(worker);

    while(st.running) {
        view_port_update(vp);
        furi_delay_ms(100);
    }

    furi_hal_serial_deinit(st.serial_handle);
    furi_hal_serial_control_release(st.serial_handle);

    infrared_worker_rx_stop(worker);
    infrared_worker_free(worker);
    gui_remove_view_port(gui, vp);
    view_port_free(vp);
    furi_mutex_free(st.mutex);
    furi_record_close(RECORD_GUI);

    return 0;
}
