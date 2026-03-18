"""
CAN Motor Log Viewer  —  chunked edition
=========================================
Designed for large files (32 MB+, millions of lines).

Strategy
--------
1. FAST INDEX SCAN  – first pass reads ONLY timestamps + byte offsets.
   No data is decoded.  Builds a sparse index every INDEX_STRIDE lines.
   Typically completes in 1-3 s even on 100 MB files.

2. ON-DEMAND CHUNK PARSE  – given [t_start, t_end] the viewer seeks
   straight to the nearest index entry, then parses only the lines in
   that window.  Parsing + rendering a typical 5-10 s chunk takes < 1 s.

3. LTTB DOWNSAMPLING  – even within a chunk, series are downsampled to
   MAX_PLOT_PTS before being handed to matplotlib so drawing is instant.

Usage
-----
    python can_log_viewer.py [optional: path/to/file.log]

Requirements
------------
    pip install matplotlib numpy
"""

import sys, os, threading, queue, bisect
from collections import defaultdict

import numpy as np
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
from matplotlib.lines import Line2D

# ═══════════════════════════════════════ constants ════════════════

INDEX_STRIDE  = 500     # store one index entry every N lines
MAX_PLOT_PTS  = 5000    # LTTB target per series
DEFAULT_CHUNK = 10.0    # seconds per chunk (user-adjustable)

MOTOR_COLORS = {
    1:'#00b4d8', 2:'#f77f00', 3:'#4cc9f0', 4:'#f72585',
    5:'#b5e48c', 6:'#9d4edd', 7:'#64d8cb', 8:'#e9c46a',
}

ERROR_NAMES = {
    (0, 0):"No Steps",            (0, 1):"Magnetic Encoder Error",
    (0, 2):"Resolver Error",      (0, 3):"Resolver Checksum Fail",
    (0, 4):"Unexpected Pos Cmd",  (0, 5):"Gate Driver Fault",
    (0, 6):"FUSE BLOWN",    (0, 7):"MA302 Init Fail",
    (0, 8):"Rotor Home Failure",  (0, 9):"Resolver Zero Failure",
    (0,10):"STALL TRIGGERED",     (0,11):"Overspeed Fault",
}
ERROR_COLORS = {
    (0, 0):'#ff6b6b', (0, 1):'#ffd166', (0, 2):'#06d6a0', (0, 3):'#118ab2',
    (0, 4):'#ef476f', (0, 5):'#ff4d4d', (0, 6):'#ff4d4d', (0, 7):'#ffa500',
    (0, 8):'#c77dff', (0, 9):'#90e0ef', (0,10):'#ff0000', (0,11):'#ff6600',
    'gate':'#ff2222',
}

# (display_label, ts_key, group_tag)
CHANNELS = {
    'rotor_velocity':    ('Rotor Velocity',    'ts_p1',  'P1'),
    'q_current':         ('Q Current',          'ts_p1',  'P1'),
    'd_current':         ('D Current',          'ts_p2',  'P2'),
    'rotor_interval':    ('Rotor Interval',      'ts_p2',  'P2'),
    'resolver_position': ('Resolver Position',   'ts_p3',  'P3'),
    'resolver_velocity': ('Resolver Velocity',   'ts_p3',  'P3'),
    'power':             ('Power',               'ts_p4',  'P4'),
    'voltage':           ('Voltage',             'ts_p4',  'P4'),
    'temperature':       ('Temperature',         'ts_p4',  'P4'),
    'velocity_command':  ('Cmd: Velocity',       'ts_vcmd','CMD'),
    'position_command':  ('Cmd: Position',       'ts_pcmd','CMD'),
}
CH_GROUPS = {
    'P1 – Velocity / Q-Cur':  ['rotor_velocity',    'q_current'],
    'P2 – Interval / D-Cur':  ['d_current',         'rotor_interval'],
    'P3 – Resolver':           ['resolver_position', 'resolver_velocity'],
    'P4 – Power / V / Temp':  ['power',             'voltage','temperature'],
    'Commands':                ['velocity_command',  'position_command'],
}

BG_DARK   = '#0d1117'
BG_PANEL  = '#161b22'
BG_WIDGET = '#21262d'
FG_TEXT   = '#c9d1d9'
FG_DIM    = '#8b949e'
FG_GREY   = '#3d444d'
ACCENT    = '#58a6ff'
BORDER    = '#30363d'
RED       = '#ff6b6b'


# ═══════════════════════════════════════ hex helpers ══════════════

def _u16(h, o): return int(h[o:o+4], 16)
def _i16(h, o):
    v = int(h[o:o+4], 16); return v - 0x10000 if v >= 0x8000 else v
def _i32(h, o):
    v = int(h[o:o+8], 16); return v - 0x100000000 if v >= 0x80000000 else v


# ═══════════════════════════════════════ index scan ═══════════════

class FileIndex:
    """
    Sparse index: list of (timestamp, byte_offset) taken every INDEX_STRIDE
    lines.  Used to seek quickly into any time window.
    """
    __slots__ = ('timestamps', 'offsets', 't_min', 't_max', 'filepath',
                 'total_lines')

    def __init__(self, filepath, progress_cb=None):
        self.filepath    = filepath
        self.timestamps  = []   # float
        self.offsets     = []   # int  (byte offset of that line's start)
        self.t_min       = None
        self.t_max       = None
        self.total_lines = 0

        file_size = os.path.getsize(filepath)

        with open(filepath, 'r', buffering=8*1024*1024, errors='replace') as fh:
            byte_pos   = 0
            line_count = 0

            for raw in fh:
                line_len    = len(raw.encode('utf-8', errors='replace'))
                line_count += 1

                # Parse timestamp only (up to first ')')
                try:
                    p = raw.index(')')
                    ts = float(raw[1:p])
                except (ValueError, IndexError):
                    byte_pos += line_len
                    continue

                if self.t_min is None or ts < self.t_min:
                    self.t_min = ts
                if self.t_max is None or ts > self.t_max:
                    self.t_max = ts

                if line_count % INDEX_STRIDE == 1:
                    self.timestamps.append(ts)
                    self.offsets.append(byte_pos)

                byte_pos += line_len

                if progress_cb and line_count % 100_000 == 0:
                    progress_cb(byte_pos, file_size, 'Indexing…')

        self.total_lines = line_count
        if self.t_min is None:
            self.t_min = self.t_max = 0.0

    def seek_offset_for(self, t_target: float) -> int:
        """Return byte offset of the index entry just before t_target."""
        if not self.timestamps:
            return 0
        idx = bisect.bisect_right(self.timestamps, t_target) - 1
        idx = max(0, idx)
        return self.offsets[idx]


# ═══════════════════════════════════════ chunk parser ════════════

def parse_chunk(filepath: str, seek_offset: int,
                t_start: float, t_end: float):
    """
    Open file at seek_offset and parse lines whose timestamp falls in
    [t_start, t_end].  Stop as soon as timestamp exceeds t_end by a
    small margin (we overshoot slightly to avoid cutting packets).

    Returns (motor_data dict, errors list).
    """
    motor_data = defaultdict(lambda: defaultdict(list))
    errors: list = []

    with open(filepath, 'rb') as fh:
        fh.seek(seek_offset)
        # wrap in text mode manually so we can do a byte seek first
        import io
        text = io.TextIOWrapper(fh, encoding='utf-8', errors='replace',
                                newline='\n')

        past_end_count = 0  # allow a few lines past t_end (packets may straddle)

        for raw in text:
            try:
                p        = raw.index(')')
                ts       = float(raw[1:p])
            except (ValueError, IndexError):
                continue

            if ts < t_start:
                continue
            if ts > t_end:
                past_end_count += 1
                if past_end_count > 50:
                    break
                continue

            # ── field extraction ──────────────────────────────────
            try:
                hash_pos = raw.index('#', p)
                sp       = raw.rindex(' ', p, hash_pos)
                raw_id   = raw[sp+1:hash_pos]
                data_str = raw[hash_pos+1:].rstrip()
            except (ValueError, IndexError):
                continue

            if not raw_id:
                continue
            try:
                can_id = int(raw_id, 16)
            except ValueError:
                continue

            first_nib   = (can_id >> 8) & 0xF
            motor_id    = (can_id >> 4) & 0xF
            packet_type =  can_id       & 0xF

            if motor_id < 1 or motor_id > 8:
                continue

            dlen = len(data_str)

            if first_nib == 4:   # telemetry

                if packet_type == 0 and dlen >= 4:
                    b0 = int(data_str[0:2], 16)
                    b1 = int(data_str[2:4], 16)
                    errors.append((ts, motor_id,
                                   'gate' if b0 == 0x03 else b0, b1))

                elif packet_type == 1 and dlen >= 8:
                    motor_data[motor_id]['ts_p1'].append(ts)
                    motor_data[motor_id]['rotor_velocity'].append(_i16(data_str, 0))
                    motor_data[motor_id]['q_current'].append(     _i16(data_str, 4))

                elif packet_type == 2 and dlen >= 8:
                    motor_data[motor_id]['ts_p2'].append(ts)
                    motor_data[motor_id]['rotor_interval'].append(_u16(data_str, 0))
                    motor_data[motor_id]['d_current'].append(     _i16(data_str, 4))

                elif packet_type == 3 and dlen >= 8:
                    motor_data[motor_id]['ts_p3'].append(ts)
                    motor_data[motor_id]['resolver_position'].append(_i16(data_str, 0))
                    motor_data[motor_id]['resolver_velocity'].append(_i16(data_str, 4))

                elif packet_type == 4 and dlen >= 12:
                    motor_data[motor_id]['ts_p4'].append(ts)
                    motor_data[motor_id]['power'].append(      _u16(data_str, 0))
                    motor_data[motor_id]['voltage'].append(    _u16(data_str, 4))
                    motor_data[motor_id]['temperature'].append(_u16(data_str, 8))

            elif first_nib == 0:  # commands

                if packet_type == 3 and dlen >= 8:
                    motor_data[motor_id]['ts_vcmd'].append(ts)
                    motor_data[motor_id]['velocity_command'].append(_i32(data_str, 0))

                elif packet_type == 4 and dlen >= 8:
                    motor_data[motor_id]['ts_pcmd'].append(ts)
                    motor_data[motor_id]['position_command'].append(_i32(data_str, 0))

    # list → ndarray
    for mid in motor_data:
        for k in list(motor_data[mid]):
            arr = motor_data[mid][k]
            motor_data[mid][k] = np.array(
                arr, dtype=np.float64 if k.startswith('ts_') else np.float32)

    return dict(motor_data), errors


# ═══════════════════════════════════════ LTTB ════════════════════

def lttb(x: np.ndarray, y: np.ndarray, n_out: int):
    n = len(x)
    if n <= n_out or n_out < 3:
        return x, y
    buckets = np.array_split(np.arange(1, n - 1), n_out - 2)
    idx = [0]; prev = 0
    for bucket in buckets:
        if len(bucket) == 0:
            continue
        nxt   = min(int(bucket[-1]) + 1, n - 1)
        areas = 0.5 * np.abs(
            (x[prev] - x[nxt]) * (y[bucket] - y[prev]) -
            (x[prev] - x[bucket]) * (y[nxt] - y[prev])
        )
        best = int(bucket[np.argmax(areas)])
        idx.append(best); prev = best
    idx.append(n - 1)
    return x[idx], y[idx]


# ═══════════════════════════════════════ progress dialog ═════════

class ProgressDialog(tk.Toplevel):
    def __init__(self, parent, title='Working…'):
        super().__init__(parent)
        self.title(title)
        self.configure(bg=BG_DARK)
        self.resizable(False, False)
        self.geometry('400x115')
        self.transient(parent)
        self.grab_set()

        tk.Label(self, text=title, bg=BG_DARK, fg=ACCENT,
                 font=('Consolas', 10, 'bold')).pack(pady=(16, 6))

        s = ttk.Style(self)
        s.configure('P.Horizontal.TProgressbar',
                    troughcolor=BG_WIDGET, background=ACCENT,
                    bordercolor=BORDER, lightcolor=ACCENT, darkcolor=ACCENT)
        self._pb = ttk.Progressbar(self, style='P.Horizontal.TProgressbar',
                                    orient='horizontal', length=340,
                                    mode='determinate', maximum=100)
        self._pb.pack(pady=3)
        self._lbl = tk.Label(self, text='…', bg=BG_DARK, fg=FG_DIM,
                              font=('Consolas', 8))
        self._lbl.pack()

    def set(self, pct: float, text: str = ''):
        self._pb['value'] = pct
        if text: self._lbl.config(text=text)
        self.update_idletasks()

    def close(self):
        self.grab_release()
        self.destroy()


# ═══════════════════════════════════════ main GUI ════════════════

class CANLogViewer(tk.Tk):

    def __init__(self, initial_file=None):
        super().__init__()
        self.title('CAN Motor Log Viewer')
        self.geometry('1560x960')
        self.configure(bg=BG_DARK)
        self.minsize(900, 600)

        # state
        self._index:       FileIndex | None = None
        self._motor_data:  dict = {}
        self._errors:      list = []
        self._chunk_start: float = 0.0
        self._chunk_end:   float = 0.0
        self._axes:        list  = []

        # Tk variables
        self._motor_vars   = {i: tk.BooleanVar(value=False) for i in range(1, 9)}
        self._channel_vars = {k: tk.BooleanVar(
                                value=(k in ('rotor_velocity', 'q_current')))
                              for k in CHANNELS}
        self._show_err_var = tk.BooleanVar(value=True)
        self._chunk_size   = tk.StringVar(value=str(DEFAULT_CHUNK))
        self._chunk_label  = tk.StringVar(value='—')
        self._status_var   = tk.StringVar(value='Load a .log file to begin')

        self._configure_styles()
        self._build_ui()

        if initial_file and os.path.isfile(initial_file):
            self.after(200, lambda: self._load(initial_file))

    # ─── styles ──────────────────────────────────────────────────

    def _configure_styles(self):
        s = ttk.Style(self)
        s.theme_use('default')
        s.configure('TCheckbutton', background=BG_PANEL, foreground=FG_TEXT,
                    font=('Consolas', 9), indicatorcolor=BG_WIDGET)
        s.map('TCheckbutton', background=[('active', BG_PANEL)])
        s.configure('TScrollbar', background=BG_WIDGET, troughcolor=BG_DARK,
                    bordercolor=BORDER, arrowcolor=FG_DIM)

    # ─── layout ──────────────────────────────────────────────────

    def _build_ui(self):
        # LEFT  sidebar
        sb_outer = tk.Frame(self, bg=BG_PANEL, width=280)
        sb_outer.pack(side=tk.LEFT, fill=tk.Y)
        sb_outer.pack_propagate(False)

        sb_cv = tk.Canvas(sb_outer, bg=BG_PANEL, highlightthickness=0, width=275)
        sb_sc = ttk.Scrollbar(sb_outer, orient='vertical', command=sb_cv.yview)
        sb_sc.pack(side=tk.RIGHT, fill=tk.Y)
        sb_cv.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self._inner = tk.Frame(sb_cv, bg=BG_PANEL)
        win_id = sb_cv.create_window((0, 0), window=self._inner, anchor='nw')

        self._inner.bind('<Configure>',
            lambda e: sb_cv.configure(scrollregion=sb_cv.bbox('all')))
        sb_cv.bind('<Configure>',
            lambda e: sb_cv.itemconfig(win_id, width=e.width))
        sb_cv.configure(yscrollcommand=sb_sc.set)
        self._inner.bind_all('<MouseWheel>',
            lambda e: sb_cv.yview_scroll(int(-1*(e.delta/120)), 'units'))

        self._build_sidebar(self._inner)

        # RIGHT  plot + bottom bar
        right = tk.Frame(self, bg=BG_DARK)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        self._build_plot_area(right)
        self._build_bottom_bar(right)

    # ─── sidebar helpers ─────────────────────────────────────────

    def _sep(self, p, pady=(5, 5)):
        tk.Frame(p, bg=BORDER, height=1).pack(fill=tk.X, padx=12, pady=pady)

    def _sec(self, p, text):
        tk.Label(p, text=text, bg=BG_PANEL, fg=FG_DIM,
                 font=('Consolas', 8, 'bold')).pack(anchor='w', padx=14, pady=(8,2))

    def _mk_btn(self, parent, text, cmd, accent=False, **kw):
        bg  = ACCENT    if accent else BG_WIDGET
        fg  = '#0d1117' if accent else FG_DIM
        abg = '#79c0ff' if accent else BORDER
        afg = '#0d1117' if accent else FG_TEXT
        return tk.Button(parent, text=text, command=cmd,
                         bg=bg, fg=fg,
                         font=('Consolas', 8, 'bold' if accent else 'normal'),
                         relief=tk.FLAT, cursor='hand2',
                         activebackground=abg, activeforeground=afg,
                         bd=0, **kw)

    # ─── sidebar ─────────────────────────────────────────────────

    def _build_sidebar(self, p):

        # header
        tk.Label(p, text='◈  CAN LOG VIEWER', bg='#0d1117', fg=ACCENT,
                 font=('Consolas', 12, 'bold'), pady=12).pack(fill=tk.X)

        self._mk_btn(p, '  LOAD  .LOG  FILE  ', self._open_file,
                     accent=True, padx=6, pady=5
                     ).pack(fill=tk.X, padx=14, pady=(8,3))

        self._file_lbl = tk.Label(p, text='no file loaded', bg=BG_PANEL,
                                   fg=FG_DIM, font=('Consolas', 7),
                                   wraplength=245, justify='left')
        self._file_lbl.pack(anchor='w', padx=14, pady=(0,4))

        self._sep(p)

        # ── BLCMDs ──────────────────────────────────────────────
        self._sec(p, 'BLCMDs')
        self._motor_cbs: dict[int, tk.Checkbutton] = {}
        mf = tk.Frame(p, bg=BG_PANEL)
        mf.pack(fill=tk.X, padx=14, pady=(0,4))

        for row_ids in ((1,2,3,4),(5,6,7,8)):
            row = tk.Frame(mf, bg=BG_PANEL)
            row.pack(fill=tk.X, pady=1)
            for mid in row_ids:
                cb = tk.Checkbutton(
                    row, text=f'M{mid}',
                    variable=self._motor_vars[mid],
                    bg=BG_PANEL, fg=FG_GREY,
                    disabledforeground=FG_GREY,
                    selectcolor=BG_WIDGET,
                    activebackground=BG_PANEL,
                    activeforeground=FG_GREY,
                    font=('Consolas', 9, 'bold'),
                    command=self._replot_current,
                    bd=0, highlightthickness=0,
                    state=tk.DISABLED)
                cb.pack(side=tk.LEFT, padx=(0,6))
                self._motor_cbs[mid] = cb

        br = tk.Frame(p, bg=BG_PANEL)
        br.pack(fill=tk.X, padx=14, pady=(0,4))
        self._mk_btn(br, 'all',
            lambda: (
                [self._motor_vars[i].set(True)
                 for i in range(1,9)
                 if str(self._motor_cbs[i]['state']) != 'disabled'],
                self._replot_current()),
            padx=8, pady=2).pack(side=tk.LEFT, padx=(0,4))
        self._mk_btn(br, 'none',
            lambda: (
                [self._motor_vars[i].set(False) for i in range(1,9)],
                self._replot_current()),
            padx=8, pady=2).pack(side=tk.LEFT)

        self._sep(p)

        # ── Channels ────────────────────────────────────────────
        self._sec(p, 'CHANNELS')
        cf = tk.Frame(p, bg=BG_PANEL)
        cf.pack(fill=tk.X, padx=14, pady=(0,4))

        for grp, keys in CH_GROUPS.items():
            tk.Label(cf, text=f'  {grp}', bg=BG_PANEL, fg=FG_GREY,
                     font=('Consolas', 7, 'bold')).pack(anchor='w', pady=(5,1))
            for k in keys:
                row = tk.Frame(cf, bg=BG_PANEL)
                row.pack(fill=tk.X, pady=1)
                tk.Label(row, text='▸', bg=BG_PANEL, fg=FG_GREY,
                         font=('Consolas', 8)).pack(side=tk.LEFT)
                tk.Checkbutton(row, text=CHANNELS[k][0],
                               variable=self._channel_vars[k],
                               bg=BG_PANEL, fg=FG_TEXT,
                               selectcolor=BG_WIDGET,
                               activebackground=BG_PANEL, activeforeground='white',
                               font=('Consolas', 8), anchor='w',
                               command=self._replot_current,
                               bd=0, highlightthickness=0
                               ).pack(side=tk.LEFT, fill=tk.X)

        cbr = tk.Frame(p, bg=BG_PANEL)
        cbr.pack(fill=tk.X, padx=14, pady=(2,4))
        self._mk_btn(cbr, 'all',
            lambda: ([v.set(True) for v in self._channel_vars.values()],
                     self._replot_current()),
            padx=8, pady=2).pack(side=tk.LEFT, padx=(0,4))
        self._mk_btn(cbr, 'none',
            lambda: ([v.set(False) for v in self._channel_vars.values()],
                     self._replot_current()),
            padx=8, pady=2).pack(side=tk.LEFT)

        self._sep(p)

        # ── Overlays ────────────────────────────────────────────
        self._sec(p, 'OVERLAYS')
        tk.Checkbutton(p, text='Show error markers',
                       variable=self._show_err_var,
                       bg=BG_PANEL, fg=RED, selectcolor=BG_WIDGET,
                       activebackground=BG_PANEL, activeforeground='#ff9090',
                       font=('Consolas', 8), command=self._replot_current,
                       bd=0, highlightthickness=0
                       ).pack(anchor='w', padx=14, pady=(0,4))

        self._sep(p)

        # ── Error log ───────────────────────────────────────────
        self._sec(p, 'ERRORS IN CHUNK  (click → zoom)')
        el = tk.Frame(p, bg=BG_DARK)
        el.pack(fill=tk.BOTH, padx=14, pady=(0,14), expand=True)
        sb2 = ttk.Scrollbar(el)
        sb2.pack(side=tk.RIGHT, fill=tk.Y)
        self._err_list = tk.Listbox(
            el, bg=BG_DARK, fg=RED,
            font=('Consolas', 7), height=12,
            yscrollcommand=sb2.set,
            selectbackground=BG_WIDGET,
            relief=tk.FLAT, borderwidth=0,
            highlightthickness=0, activestyle='none')
        self._err_list.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        sb2.config(command=self._err_list.yview)
        self._err_list.bind('<<ListboxSelect>>', self._err_zoom)

    # ─── plot area ───────────────────────────────────────────────

    def _build_plot_area(self, parent):
        plt.rcParams.update({'font.family':'monospace', 'axes.titlesize':9})
        self._fig    = plt.Figure(facecolor=BG_DARK, tight_layout=False)
        self._canvas = FigureCanvasTkAgg(self._fig, master=parent)
        self._canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        self._draw_empty('Load a .log file to begin')

    def _draw_empty(self, msg=''):
        self._fig.clear()
        ax = self._fig.add_subplot(111)
        ax.set_facecolor(BG_DARK)
        if msg:
            ax.text(0.5, 0.5, msg, transform=ax.transAxes,
                    ha='center', va='center', color=FG_GREY,
                    fontsize=14, fontfamily='monospace')
        self._style_ax(ax)
        self._canvas.draw()

    # ─── bottom navigation bar ───────────────────────────────────

    def _build_bottom_bar(self, parent):
        bar = tk.Frame(parent, bg=BG_PANEL, height=46)
        bar.pack(side=tk.BOTTOM, fill=tk.X)
        bar.pack_propagate(False)

        # matplotlib toolbar (compact)
        tb_wrap = tk.Frame(bar, bg=BG_PANEL)
        tb_wrap.pack(side=tk.RIGHT, fill=tk.Y, padx=(0,6))
        toolbar = NavigationToolbar2Tk(self._canvas, tb_wrap)
        toolbar.config(bg=BG_PANEL)
        for child in toolbar.winfo_children():
            try: child.config(bg=BG_PANEL, fg=FG_TEXT,
                              activebackground=BG_WIDGET, relief=tk.FLAT)
            except Exception: pass
        toolbar.update()

        # ← chunk size → controls on the left
        nav = tk.Frame(bar, bg=BG_PANEL)
        nav.pack(side=tk.LEFT, fill=tk.Y, padx=8)

        # Row 1: Prev / chunk label / Next
        r1 = tk.Frame(nav, bg=BG_PANEL)
        r1.pack(side=tk.TOP, pady=(5,2))

        self._btn_prev = self._mk_btn(r1, '◀  Prev', self._prev_chunk,
                                       padx=10, pady=3)
        self._btn_prev.pack(side=tk.LEFT, padx=(0,6))

        tk.Label(r1, textvariable=self._chunk_label,
                 bg=BG_PANEL, fg=ACCENT, font=('Consolas', 9, 'bold'),
                 width=36, anchor='center').pack(side=tk.LEFT)

        self._btn_next = self._mk_btn(r1, 'Next  ▶', self._next_chunk,
                                       padx=10, pady=3)
        self._btn_next.pack(side=tk.LEFT, padx=(6,0))

        # Row 2: chunk size + goto
        r2 = tk.Frame(nav, bg=BG_PANEL)
        r2.pack(side=tk.TOP, pady=(0,4))

        tk.Label(r2, text='Chunk size (s):', bg=BG_PANEL, fg=FG_DIM,
                 font=('Consolas', 8)).pack(side=tk.LEFT, padx=(0,4))
        tk.Entry(r2, textvariable=self._chunk_size, width=7,
                 bg=BG_WIDGET, fg=FG_TEXT, insertbackground=ACCENT,
                 font=('Consolas', 8), relief=tk.FLAT,
                 highlightthickness=1, highlightcolor=ACCENT,
                 highlightbackground=BORDER).pack(side=tk.LEFT, padx=(0,8))

        tk.Label(r2, text='Jump to t=', bg=BG_PANEL, fg=FG_DIM,
                 font=('Consolas', 8)).pack(side=tk.LEFT, padx=(0,4))
        self._goto_var = tk.StringVar()
        tk.Entry(r2, textvariable=self._goto_var, width=14,
                 bg=BG_WIDGET, fg=FG_TEXT, insertbackground=ACCENT,
                 font=('Consolas', 8), relief=tk.FLAT,
                 highlightthickness=1, highlightcolor=ACCENT,
                 highlightbackground=BORDER).pack(side=tk.LEFT, padx=(0,4))
        self._mk_btn(r2, 'Go', self._goto_time,
                     accent=True, padx=8, pady=2).pack(side=tk.LEFT)

        # Status bar
        tk.Label(parent, textvariable=self._status_var,
                 bg='#0d1117', fg=FG_DIM, font=('Consolas', 7),
                 anchor='w', padx=8).pack(side=tk.BOTTOM, fill=tk.X)

    # ─── file loading ────────────────────────────────────────────

    def _open_file(self):
        fp = filedialog.askopenfilename(
            title='Open CAN Log File',
            filetypes=[('Log files','*.log'),('Text files','*.txt'),
                       ('All files','*.*')])
        if fp:
            self._load(fp)

    def _load(self, filepath: str):
        self._file_lbl.config(text=os.path.basename(filepath))
        self._draw_empty('Building index…')
        self._set_status('Scanning file — building time index…')

        prog = ProgressDialog(self,
            f'Indexing  {os.path.basename(filepath)}')
        rq: queue.Queue = queue.Queue()

        def _worker():
            try:
                idx = FileIndex(
                    filepath,
                    progress_cb=lambda d, t, msg:
                        rq.put(('progress',
                                min(99.0, 100.0*d/t) if t else 0, msg)))
                rq.put(('done', idx))
            except Exception as exc:
                rq.put(('error', str(exc)))

        threading.Thread(target=_worker, daemon=True).start()

        def _poll():
            try:
                msg = rq.get_nowait()
            except queue.Empty:
                self.after(80, _poll); return

            if msg[0] == 'progress':
                prog.set(msg[1], msg[2]); self.after(80, _poll)
            elif msg[0] == 'done':
                prog.close()
                self._on_index_done(filepath, msg[1])
            else:
                prog.close()
                messagebox.showerror('Error', msg[1])

        self.after(80, _poll)

    def _on_index_done(self, filepath: str, idx: FileIndex):
        self._index        = idx
        self._filepath     = filepath
        self._chunk_start  = idx.t_min
        self._chunk_end    = idx.t_min + self._get_chunk_size()

        dur = idx.t_max - idx.t_min
        self._set_status(
            f'{os.path.basename(filepath)}  |  '
            f'duration {dur:.2f} s  |  '
            f'{idx.total_lines:,} lines  |  '
            f'{idx.total_lines//INDEX_STRIDE:,} index entries')

        # We don't know which motors are present until we parse a chunk,
        # so reset all to disabled until first chunk is loaded.
        for mid in range(1, 9):
            self._motor_cbs[mid].config(state=tk.DISABLED, fg=FG_GREY)
            self._motor_vars[mid].set(False)

        self._load_and_plot_chunk(self._chunk_start, self._chunk_end)

    # ─── chunk navigation ────────────────────────────────────────

    def _get_chunk_size(self) -> float:
        try:
            v = float(self._chunk_size.get())
            return v if v > 0 else DEFAULT_CHUNK
        except ValueError:
            return DEFAULT_CHUNK

    def _next_chunk(self):
        if self._index is None:
            return
        cs  = self._get_chunk_size()
        new_start = self._chunk_end
        new_end   = new_start + cs
        if new_start >= self._index.t_max:
            self._set_status('Already at end of file.')
            return
        self._load_and_plot_chunk(new_start, min(new_end, self._index.t_max))

    def _prev_chunk(self):
        if self._index is None:
            return
        cs        = self._get_chunk_size()
        new_end   = self._chunk_start
        new_start = new_end - cs
        if new_end <= self._index.t_min:
            self._set_status('Already at start of file.')
            return
        self._load_and_plot_chunk(max(new_start, self._index.t_min), new_end)

    def _goto_time(self):
        if self._index is None:
            return
        try:
            t = float(self._goto_var.get())
        except ValueError:
            messagebox.showerror('Invalid', 'Enter a numeric timestamp (absolute s)')
            return
        cs = self._get_chunk_size()
        t  = max(self._index.t_min, min(t, self._index.t_max))
        self._load_and_plot_chunk(t, min(t + cs, self._index.t_max))

    # ─── chunk loading  (threaded) ───────────────────────────────

    def _load_and_plot_chunk(self, t_start: float, t_end: float):
        if self._index is None:
            return

        self._set_status(f'Loading chunk  {t_start:.3f} → {t_end:.3f} s …')
        self._draw_empty(f'Loading  {t_start:.2f} s → {t_end:.2f} s …')

        rq: queue.Queue = queue.Queue()
        seek_off = self._index.seek_offset_for(t_start)

        def _worker():
            try:
                data, errs = parse_chunk(self._filepath, seek_off, t_start, t_end)
                rq.put(('done', data, errs, t_start, t_end))
            except Exception as exc:
                rq.put(('error', str(exc)))

        threading.Thread(target=_worker, daemon=True).start()

        def _poll():
            try:
                msg = rq.get_nowait()
            except queue.Empty:
                self.after(50, _poll); return
            if msg[0] == 'done':
                _, data, errs, ts, te = msg
                self._chunk_start = ts
                self._chunk_end   = te
                self._on_chunk_done(data, errs, ts, te)
            else:
                messagebox.showerror('Parse error', msg[1])

        self.after(50, _poll)

    def _on_chunk_done(self, motor_data, errors, t_start, t_end):
        self._motor_data = motor_data
        self._errors     = errors

        # Update BLCMD checkboxes based on what's in THIS chunk
        present = set(motor_data.keys())
        # Also keep enabled any motor that was already enabled from a prior chunk
        for mid in range(1, 9):
            cb  = self._motor_cbs[mid]
            var = self._motor_vars[mid]
            if mid in present:
                color = MOTOR_COLORS[mid]
                if str(cb['state']) == 'disabled':
                    cb.config(state=tk.NORMAL, fg=color, activeforeground=color)
                    var.set(True)
            # Don't disable motors that were active — they might re-appear next chunk

        # Error list
        self._err_list.delete(0, tk.END)
        t0 = self._index.t_min if self._index else t_start
        for ts, mid, b0, b1 in errors:
            name = ('Gate Driver Fault' if b0 == 'gate'
                    else ERROR_NAMES.get((b0, b1), f'Err[{b0:02X},{b1:02X}]'))
            self._err_list.insert(tk.END, f't={ts-t0:>9.3f}s  M{mid}  {name}')

        # Chunk position label
        t0 = self._index.t_min if self._index else 0.0
        total = (self._index.t_max - self._index.t_min) if self._index else 1.0
        pct   = 100.0 * (t_start - t0) / total if total else 0.0
        self._chunk_label.set(
            f'{t_start-t0:.3f} s – {t_end-t0:.3f} s   '
            f'(+{t_end-t_start:.1f} s,  {pct:.1f}% into file)')

        n_pts = sum(len(v) for md in motor_data.values()
                    for k, v in md.items() if not k.startswith('ts_'))
        self._set_status(
            f'Chunk  {t_start-t0:.3f} – {t_end-t0:.3f} s  |  '
            f'{n_pts:,} data points  |  '
            f'{len(errors)} errors')

        self._replot_current()

    # ─── replot (uses already-parsed chunk data) ─────────────────

    def _replot_current(self):
        if not self._motor_data and self._index is None:
            return

        active_ch  = [k for k, v in self._channel_vars.items() if v.get()]
        active_mot = [i for i, v in self._motor_vars.items()   if v.get()]

        self._fig.clear()
        self._axes = []

        if not active_ch or not active_mot:
            self._draw_empty('Select at least one BLCMD and channel')
            return

        n  = len(active_ch)
        t0 = self._index.t_min if self._index else self._chunk_start

        gs = self._fig.add_gridspec(
            n, 1, hspace=0.07,
            left=0.08, right=0.97, top=0.95, bottom=0.04)

        for i, ch in enumerate(active_ch):
            ax = self._fig.add_subplot(
                gs[i], sharex=self._axes[0] if self._axes else None)
            self._axes.append(ax)
            self._style_ax(ax)

            ch_label, ts_key, _ = CHANNELS[ch]
            ax.set_ylabel(ch_label, color=FG_DIM, fontsize=7.5,
                          fontfamily='monospace', labelpad=3)
            ax.tick_params(axis='y', labelsize=7, labelcolor=FG_DIM)

            if i < n - 1:
                plt.setp(ax.get_xticklabels(), visible=False)
            else:
                ax.set_xlabel('Time (s, relative to file start)',
                              color=FG_DIM, fontsize=8, fontfamily='monospace')
                ax.tick_params(axis='x', labelsize=7, labelcolor=FG_DIM)

            has_data = False
            for mid in active_mot:
                if mid not in self._motor_data:
                    continue
                md = self._motor_data[mid]
                if ts_key not in md or ch not in md:
                    continue
                ts_abs = md[ts_key]
                vals   = md[ch]
                if len(ts_abs) == 0:
                    continue

                ts_rel = (ts_abs - t0).astype(np.float64)
                v_plot = vals.astype(np.float64)
                ts_rel, v_plot = lttb(ts_rel, v_plot, MAX_PLOT_PTS)

                ax.plot(ts_rel, v_plot,
                        color=MOTOR_COLORS[mid], linewidth=0.9,
                        label=f'M{mid}', alpha=0.93, rasterized=True)
                has_data = True

            if not has_data:
                ax.text(0.5, 0.5, 'no data in this chunk',
                        transform=ax.transAxes, ha='center', va='center',
                        color=FG_GREY, fontsize=8, fontfamily='monospace')

            if self._show_err_var.get():
                self._draw_errors(ax, active_mot, t0, show_legend=(i == 0))

            seen: dict = {}
            for h, l in zip(*ax.get_legend_handles_labels()):
                if l not in seen: seen[l] = h
            if seen:
                ax.legend(seen.values(), seen.keys(),
                          loc='upper right', fontsize=7,
                          facecolor=BG_WIDGET, edgecolor=BORDER,
                          labelcolor=FG_TEXT, handlelength=1.5,
                          borderpad=0.4, labelspacing=0.3)

        fname = os.path.basename(self._filepath) if hasattr(self,'_filepath') else ''
        t0_   = self._index.t_min if self._index else 0.0
        self._fig.suptitle(
            f'{fname}   '
            f'[{self._chunk_start-t0_:.3f} s – {self._chunk_end-t0_:.3f} s  '
            f'(rel)]',
            color=ACCENT, fontsize=9, fontfamily='monospace', x=0.53)

        self._canvas.draw()

    # ─── error overlay ───────────────────────────────────────────

    def _draw_errors(self, ax, active_mot, t0, show_legend=False):
        """
        Draw all error markers with two vectorised calls per colour group
        instead of one axvline+plot call per error.  Scales to thousands of
        errors without blocking.
        """
        if not self._errors:
            return

        # ── bucket errors by (key, color) ────────────────────────
        # buckets: color_str → {'times': [], 'name': str}
        buckets: dict = {}
        seen: dict    = {}   # key → (name, color)  for legend

        for ts, mid, b0, b1 in self._errors:
            if mid not in active_mot:
                continue
            key   = 'gate' if b0 == 'gate' else (b0, b1)
            name  = ('Gate Driver Fault' if b0 == 'gate'
                     else ERROR_NAMES.get(key, f'Err[{b0:02X},{b1:02X}]'))
            color = ERROR_COLORS.get(key, '#ffffff')
            t_rel = ts - t0

            if color not in buckets:
                buckets[color] = {'times': [], 'name': name}
            buckets[color]['times'].append(t_rel)
            seen.setdefault(key, (name, color))

        if not buckets:
            return

        # Read y-range ONCE after data has been plotted
        ylim  = ax.get_ylim()
        yspan = ylim[1] - ylim[0]

        # Draw a rug plot: short ticks pinned to the bottom 4% of the axes.
        # They never overlap the signal regardless of error density.
        rug_top = ylim[0] + 0.04 * yspan if yspan > 0 else ylim[0]

        for color, info in buckets.items():
            times = np.asarray(info['times'])

            # Short rug ticks along the bottom — one vlines() call per colour
            ax.vlines(times, ymin=ylim[0], ymax=rug_top,
                      colors=color, linewidth=1.2,
                      linestyles='solid', alpha=0.7, zorder=3)

        if show_legend and seen:
            handles = [Line2D([0],[0], color=c, linewidth=1.5,
                              linestyle='--', label=n)
                       for _, (n, c) in seen.items()]
            ax.legend(handles=handles, loc='upper left', fontsize=6.5,
                      facecolor=BG_WIDGET, edgecolor='#ff4d4d',
                      labelcolor='#ff9090',
                      title='⚠ Errors', title_fontsize=6.5,
                      borderpad=0.4, labelspacing=0.3)

    # ─── error list click ────────────────────────────────────────

    def _err_zoom(self, _event):
        sel = self._err_list.curselection()
        if not sel or not self._errors:
            return
        idx = sel[0]
        if idx >= len(self._errors):
            return
        ts = self._errors[idx][0]
        cs = self._get_chunk_size()
        # centre the chunk around the error
        new_s = ts - cs * 0.3
        new_e = new_s + cs
        if self._index:
            new_s = max(self._index.t_min, new_s)
            new_e = min(self._index.t_max, new_e)
        self._load_and_plot_chunk(new_s, new_e)

    # ─── helpers ─────────────────────────────────────────────────

    def _set_status(self, msg: str):
        self._status_var.set(msg)
        self.update_idletasks()

    @staticmethod
    def _style_ax(ax):
        ax.set_facecolor(BG_DARK)
        ax.tick_params(colors=FG_DIM)
        ax.grid(True, color=BG_WIDGET, linewidth=0.6)
        ax.grid(True, which='minor', color='#161b22', linewidth=0.3, alpha=0.5)
        for s in ax.spines.values():
            s.set_color(BORDER); s.set_linewidth(0.7)


# ═══════════════════════════════════════ entry point ══════════════

if __name__ == '__main__':
    CANLogViewer(
        initial_file=sys.argv[1] if len(sys.argv) > 1 else None
    ).mainloop()