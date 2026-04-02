import pretty_midi

# Load MIDI file
midi_data = pretty_midi.PrettyMIDI('Untitled.mid')

# Iterate through instruments and notes
for instrument in midi_data.instruments:
    for note in instrument.notes:
        start_sec = note.start
        end_sec = note.end
        duration = end_sec - start_sec
        pitch_hz = pretty_midi.note_number_to_hz(note.pitch)
        print(f"Pitch: {pitch_hz}, Start: {start_sec:.2f}s, End: {end_sec:.2f}s, Duration: {duration:.2f}s")

# Pause lengths are differences between end of one note and start of next
# (Requires sorting notes by start time)
