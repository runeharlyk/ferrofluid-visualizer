import numpy as np
import matplotlib.pyplot as plt
import sounddevice as sd
from scipy.fftpack import fft
from matplotlib.animation import FuncAnimation


def real_time_audio(sampling_rate, window_size):
    window_samples = int(window_size * sampling_rate)
    stream = sd.InputStream(
        samplerate=sampling_rate, channels=1, blocksize=window_samples
    )
    stream.start()

    fig, (ax_time, ax_freq) = plt.subplots(2, 1, figsize=(10, 8))

    # Initialize plots
    x_time = np.linspace(0, window_size, window_samples)
    (line_time,) = ax_time.plot(x_time, np.zeros(window_samples))
    ax_time.set_title("Time-Domain Signal")
    ax_time.set_xlabel("Time (s)")
    ax_time.set_ylabel("Amplitude")
    ax_time.set_ylim(-1, 1)

    freqs = np.fft.fftfreq(window_samples, d=1 / sampling_rate)[: window_samples // 2]
    (line_freq,) = ax_freq.plot(freqs, np.zeros(window_samples // 2))
    ax_freq.set_title("Frequency-Domain Signal (FFT)")
    ax_freq.set_xlabel("Frequency (Hz)")
    ax_freq.set_ylabel("Amplitude")
    ax_freq.set_xlim(0, 200)
    ax_freq.set_ylim(0, 1)

    beat_text = ax_time.text(
        0.5, 0.9, "", transform=ax_time.transAxes, ha="center", fontsize=14, color="red"
    )

    energy_history = []

    def update(frame):
        nonlocal energy_history
        audio_data, _ = stream.read(window_samples)
        audio_data = audio_data.flatten()

        # Compute FFT
        fft_result = np.abs(fft(audio_data)[: window_samples // 2])

        # Compute energy
        energy = np.sum(audio_data**2)
        energy_history.append(energy)
        if len(energy_history) > 50:  # Limit history to the last 50 frames
            energy_history.pop(0)

        # Threshold for beat detection
        avg_energy = np.mean(energy_history)
        beat_detected = energy > 1.5 * avg_energy

        # Update time-domain plot
        line_time.set_ydata(audio_data)

        # Update frequency-domain plot
        line_freq.set_ydata(fft_result)

        # Display beat detection
        if beat_detected:
            beat_text.set_text("Beat Detected!")
        else:
            beat_text.set_text("")

        return line_time, line_freq, beat_text

    ani = FuncAnimation(fig, update, interval=int(window_size * 1000), blit=True)
    plt.tight_layout()
    plt.show()

    stream.stop()


if __name__ == "__main__":
    sampling_rate = int(input("Enter sampling rate (Hz): "))
    window_size = float(input("Enter window size (seconds): "))
    real_time_audio(sampling_rate, window_size)
