import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider, RectangleSelector
import os
import argparse

# --- Constants ---
INITIAL_WINDOW_SEC = 2.0
DTYPE = np.float32
BYTES_PER_SAMPLE = np.dtype(DTYPE).itemsize
MAX_WIDTH_SEC = 2.0  # Max zoom out (increase if you want)

class WaveformVisualizer:
    def __init__(self, filenames, rate, min_zoom_samples=100):
        self.rate = rate
        self.navigating = False
        self.filenames = filenames
        self.min_zoom_samples = min_zoom_samples
        self.min_width_sec = min_zoom_samples / rate

        self.mapped_files = []
        self.lines = []
        self.max_samples = 0

        # Load files (raw float32)
        for f in filenames:
            try:
                fsize = os.path.getsize(f)
                samples = fsize // BYTES_PER_SAMPLE
                if samples <= 0:
                    raise RuntimeError("file too small / empty")

                mm = np.memmap(f, dtype=DTYPE, mode='r', shape=(samples,))
                self.mapped_files.append((mm, os.path.basename(f)))
                self.max_samples = max(self.max_samples, samples)
            except Exception as e:
                print(f"Error opening {f}: {e}")

        if not self.mapped_files:
            raise SystemExit("No files loaded.")

        self.duration_sec = self.max_samples / self.rate
        self.setup_ui()

    def setup_ui(self):
        self.fig, self.ax = plt.subplots(figsize=(12, 6))
        plt.subplots_adjust(left=0.05, right=0.95, top=0.95, bottom=0.15)

        for _, name in self.mapped_files:
            line, = self.ax.plot([], [], linewidth=0.8, label=name)
            self.lines.append(line)

        self.ax.grid(True, which='both', linestyle=':', alpha=0.5)
        self.ax.set_xlabel("Time (s)")
        self.ax.set_ylabel("Amplitude (float32)")
        self.ax.legend(loc='upper right', fontsize='x-small')

        ax_slider = plt.axes([0.15, 0.05, 0.7, 0.03])
        self.slider = Slider(ax_slider, 'Time', 0, self.duration_sec, valinit=0)
        self.slider.on_changed(self.update_slider)

        self.fig.canvas.mpl_connect('scroll_event', self.on_scroll)
        self.fig.canvas.mpl_connect('key_press_event', self.on_key)
        self.ax.callbacks.connect('xlim_changed', self.on_xlim_changed)

        self.rs = RectangleSelector(
            self.ax, self.on_select,
            useblit=True,
            button=[1],
            minspanx=5, minspany=5,
            spancoords='pixels',
            interactive=False
        )

        self.update_view(0, INITIAL_WINDOW_SEC)
        plt.show()

    def get_chunk(self, start_time, window_duration):
        start_sample = max(0, int(start_time * self.rate))
        end_sample = int((start_time + window_duration) * self.rate)

        global_min_y, global_max_y = 1e9, -1e9
        has_data = False

        for line, (mm, _) in zip(self.lines, self.mapped_files):
            if start_sample >= mm.size:
                line.set_data([], [])
                continue

            safe_end = min(end_sample, mm.size)
            if safe_end <= start_sample:
                line.set_data([], [])
                continue

            chunk = mm[start_sample:safe_end]

            if chunk.size > 0:
                t_origin = start_sample / self.rate
                t_axis = np.arange(chunk.size, dtype=np.float64) / self.rate + t_origin

                line.set_data(t_axis, chunk)

                if chunk.size < 300:
                    line.set_marker('.')
                    line.set_markersize(3)
                else:
                    line.set_marker("")

                global_min_y = min(global_min_y, float(np.min(chunk)))
                global_max_y = max(global_max_y, float(np.max(chunk)))
                has_data = True
            else:
                line.set_data([], [])

        return has_data, global_min_y, global_max_y

    def update_view(self, start_time, width):
        if self.navigating:
            return
        self.navigating = True
        try:
            width = max(self.min_width_sec, min(width, MAX_WIDTH_SEC))
            start_time = max(0, min(start_time, self.duration_sec))

            has_data, min_y, max_y = self.get_chunk(start_time, width)
            self.ax.set_xlim(start_time, start_time + width)

            if has_data and max_y > min_y:
                max_val = max(abs(min_y), abs(max_y))
                if max_val < 1e-6:
                    max_val = 0.01
                else:
                    max_val *= 1.05
                self.ax.set_ylim(-max_val, max_val)
            else:
                self.ax.set_ylim(-1.0, 1.0)

            self.fig.canvas.draw_idle()
        finally:
            self.navigating = False

    def update_slider(self, val):
        if self.navigating:
            return
        xlim = self.ax.get_xlim()
        width = xlim[1] - xlim[0]
        if width <= 0:
            width = INITIAL_WINDOW_SEC
        self.update_view(val, width)

    def on_scroll(self, event):
        if event.inaxes != self.ax:
            return

        xlim = self.ax.get_xlim()
        cur_width = xlim[1] - xlim[0]
        if cur_width <= 0:
            return

        scale_factor = 0.8 if event.button == 'up' else 1.2
        new_width = cur_width * scale_factor
        new_width = max(self.min_width_sec, min(new_width, MAX_WIDTH_SEC))

        rel_x = (event.xdata - xlim[0]) / cur_width if event.xdata is not None else 0.5
        new_start = (event.xdata if event.xdata is not None else (xlim[0] + xlim[1]) / 2) - (new_width * rel_x)
        new_start = max(0, min(new_start, self.duration_sec - new_width))

        self.ax.set_xlim(new_start, new_start + new_width)
        self.slider.set_val(new_start)

    def on_xlim_changed(self, ax):
        if self.navigating:
            return
        self.navigating = True
        try:
            xlim = ax.get_xlim()
            start_time = xlim[0]
            width = xlim[1] - xlim[0]
            self.get_chunk(start_time, width)

            old_eventson = self.slider.eventson
            self.slider.eventson = False
            self.slider.set_val(start_time)
            self.slider.eventson = old_eventson
        finally:
            self.navigating = False

    def on_key(self, event):
        if self.navigating:
            return

        # Space = autoscale Y for current view
        if event.key == ' ':
            xlim = self.ax.get_xlim()
            self.update_view(xlim[0], xlim[1] - xlim[0])
            return

        self.navigating = True
        try:
            xlim = self.ax.get_xlim()
            ylim = self.ax.get_ylim()
            width = xlim[1] - xlim[0]
            height = ylim[1] - ylim[0]
            changed = False

            if event.key == 'right':
                shift = width * 0.25
                self.ax.set_xlim(xlim[0] + shift, xlim[1] + shift)
                changed = True
            elif event.key == 'left':
                shift = width * 0.25
                self.ax.set_xlim(xlim[0] - shift, xlim[1] - shift)
                changed = True
            elif event.key == 'up':
                shift = height * 0.25
                self.ax.set_ylim(ylim[0] + shift, ylim[1] + shift)
                changed = True
            elif event.key == 'down':
                shift = height * 0.25
                self.ax.set_ylim(ylim[0] - shift, ylim[1] - shift)
                changed = True
            elif event.key == 'pagedown':
                center = (xlim[0] + xlim[1]) / 2
                new_width = max(width * 0.5, self.min_width_sec)
                new_start = max(0, min(center - new_width / 2, self.duration_sec - new_width))
                self.ax.set_xlim(new_start, new_start + new_width)
                changed = True
            elif event.key == 'pageup':
                center = (xlim[0] + xlim[1]) / 2
                new_width = min(width * 2.0, MAX_WIDTH_SEC)
                new_start = max(0, min(center - new_width / 2, self.duration_sec - new_width))
                self.ax.set_xlim(new_start, new_start + new_width)
                changed = True

            if changed:
                new_xlim = self.ax.get_xlim()
                self.get_chunk(new_xlim[0], new_xlim[1] - new_xlim[0])

                old_eventson = self.slider.eventson
                self.slider.eventson = False
                self.slider.set_val(new_xlim[0])
                self.slider.eventson = old_eventson

                self.fig.canvas.draw_idle()
        finally:
            self.navigating = False

    def on_select(self, eclick, erelease):
        if self.navigating:
            return
        if eclick.xdata is None or erelease.xdata is None:
            return

        x1, y1 = eclick.xdata, eclick.ydata
        x2, y2 = erelease.xdata, erelease.ydata

        start_time = min(x1, x2)
        end_time = max(x1, x2)
        width = end_time - start_time

        if width < self.min_width_sec:
            center = (start_time + end_time) / 2
            width = self.min_width_sec
            start_time = center - width / 2

        width = max(self.min_width_sec, min(width, MAX_WIDTH_SEC))
        start_time = max(0, min(start_time, self.duration_sec - width))

        self.navigating = True
        try:
            self.get_chunk(start_time, width)
            self.ax.set_xlim(start_time, start_time + width)

            if y1 is not None and y2 is not None:
                y_min = min(y1, y2)
                y_max = max(y1, y2)
                if y_max > y_min:
                    self.ax.set_ylim(y_min, y_max)

            self.fig.canvas.draw_idle()

            old_eventson = self.slider.eventson
            self.slider.eventson = False
            self.slider.set_val(start_time)
            self.slider.eventson = old_eventson
        finally:
            self.navigating = False


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Raw float32 audio waveform visualizer (mmap)")
    parser.add_argument('files', nargs='+', help="Input .raw files (float32)")
    parser.add_argument('--rate', type=int, default=44100, help="Sample rate (Hz)")
    parser.add_argument('--min-zoom-samples', type=int, default=100, help="Minimum samples when zoomed in")
    parser.add_argument('--max-width-sec', type=float, default=2.0, help="Max zoom-out window (sec)")
    args = parser.parse_args()

    # allow override of MAX_WIDTH_SEC from CLI
    MAX_WIDTH_SEC = max(0.001, float(args.max_width_sec))

    WaveformVisualizer(args.files, args.rate, args.min_zoom_samples)
