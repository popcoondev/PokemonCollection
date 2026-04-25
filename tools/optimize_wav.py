#!/usr/bin/env python3
"""Downsample PCM16 WAV files for lighter in-device playback.

Examples:
  python3 tools/optimize_wav.py \
      --input data/sd/pokemon/bgm/type_matchup_loop.wav \
      --output /tmp/type_matchup_loop_22050_mono.wav \
      --sample-rate 22050 \
      --channels mono

  python3 tools/optimize_wav.py \
      --input in.wav \
      --output out.wav \
      --sample-rate 16000 \
      --channels stereo \
      --volume 0.85
"""

from __future__ import annotations

import argparse
import audioop
import pathlib
import sys
import wave


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert PCM16 WAV files to a lighter WAV for embedded playback."
    )
    parser.add_argument("--input", required=True, help="Source WAV path")
    parser.add_argument("--output", required=True, help="Destination WAV path")
    parser.add_argument(
        "--sample-rate",
        type=int,
        default=22050,
        help="Output sample rate. Default: 22050",
    )
    parser.add_argument(
        "--channels",
        choices=("mono", "stereo"),
        default="mono",
        help="Output channel count. Default: mono",
    )
    parser.add_argument(
        "--volume",
        type=float,
        default=1.0,
        help="Multiply sample amplitude. Default: 1.0",
    )
    return parser.parse_args()


def fail(message: str) -> int:
    print(f"error: {message}", file=sys.stderr)
    return 1


def main() -> int:
    args = parse_args()

    src = pathlib.Path(args.input)
    dst = pathlib.Path(args.output)
    dst.parent.mkdir(parents=True, exist_ok=True)

    if args.sample_rate <= 0:
        return fail("--sample-rate must be positive")
    if args.volume <= 0:
        return fail("--volume must be positive")

    try:
        with wave.open(str(src), "rb") as wav_in:
            channels = wav_in.getnchannels()
            sample_width = wav_in.getsampwidth()
            sample_rate = wav_in.getframerate()
            frame_count = wav_in.getnframes()
            pcm = wav_in.readframes(frame_count)
    except FileNotFoundError:
        return fail(f"input not found: {src}")
    except wave.Error as exc:
        return fail(f"failed to read WAV: {exc}")

    if sample_width != 2:
        return fail("only PCM16 WAV input is supported")
    if channels not in (1, 2):
        return fail("only mono or stereo WAV input is supported")

    target_channels = 1 if args.channels == "mono" else 2

    if channels == 2 and target_channels == 1:
        pcm = audioop.tomono(pcm, 2, 0.5, 0.5)
    elif channels == 1 and target_channels == 2:
        pcm = audioop.tostereo(pcm, 2, 1.0, 1.0)

    if sample_rate != args.sample_rate:
        pcm, _ = audioop.ratecv(
            pcm,
            2,
            target_channels,
            sample_rate,
            args.sample_rate,
            None,
        )

    if args.volume != 1.0:
        pcm = audioop.mul(pcm, 2, args.volume)

    try:
        with wave.open(str(dst), "wb") as wav_out:
            wav_out.setnchannels(target_channels)
            wav_out.setsampwidth(2)
            wav_out.setframerate(args.sample_rate)
            wav_out.writeframes(pcm)
    except wave.Error as exc:
        return fail(f"failed to write WAV: {exc}")

    frames = len(pcm) // (2 * target_channels)
    duration = frames / float(args.sample_rate)
    print(
        f"wrote {dst} ({args.sample_rate}Hz, {target_channels}ch, {duration:.2f}s)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
