const sampleRate = 8000;
const bitsPerSample = 16;
const channels = 1;

// This file contains methods for converting a raw PCM data stream into a WAV file for processing by text to speech on Cloudflare.

/**
 * 
 * @param {DataView} view 
 * @param {number} offset 
 * @param {string} string 
 */
function writeString(view, offset, string) {

    for (let i = 0; i < string.length; i++) {
        view.setUint8(offset + i, string.charCodeAt(i));
    }
    
}

/**
 * 
 * @param {ArrayBuffer} pcmData 
 * @returns {Uint8Array}
 */
export default function (pcmData) {
    
    const header = new ArrayBuffer(44);
    const view = new DataView(header);

    writeString(view, 0, "RIFF");
    view.setUint32(4, (header.byteLength + pcmData.byteLength) - 8, true); // Size of remaining data
    writeString(view, 8, "WAVE");// Format
    writeString(view, 12, "fmt "); // subChunk1Id
    view.setUint32(16, 16, true); // subChunk1 size
    view.setUint16(20, 1, true); // Audio format, 1 is PCM
    view.setUint16(22, channels, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, (sampleRate * bitsPerSample * channels) / 8, true); // Byte rate
    view.setUint16(32, (bitsPerSample * channels) / 8, true); // Block align
    view.setUint16(34, bitsPerSample, true);
    writeString(view, 36, "data"); // SubChunk2id
    view.setUint32(40, (header.byteLength + pcmData.byteLength) - 44, true);
    
    const wavBytes = new Uint8Array(header.byteLength + pcmData.byteLength)
    wavBytes.set(new Uint8Array(header), 0);
    wavBytes.set(new Uint8Array(pcmData), header.byteLength);

    return wavBytes

}