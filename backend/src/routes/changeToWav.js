const sampleRate = 16000;
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

export default {
    /**
     * 
     * @param {Request} request
     * @returns {Response}
     */
    async changePCMToWav(request) {
        
        /** @type {ReadableStream<Uint8Array>} */
        const requestBody = request.body();
        const pcmData = await requestBody.getReader().read()
        if (pcmData == null) {
            return new Response.error();
        }

        const subChunk2Size = (pcmData.value.length / bitsPerSample) * channels * (bitsPerSample / 8)

        // Create WAV file in memory.

        const header = new ArrayBuffer(44);
        const view = new DataView(header);

        writeString(view, 0, "RIFF");
        view.setUint32(4, 36 + subChunk2Size, true); // Chunk size
        writeString(view, 8, "WAVE");// Format
        writeString(view, 12, "fmt"); // subChunk1Id
        view.setUint32(16, 16, true); // subChunk1 size
        view.setUint16(20, 1, true); // Audio format
        view.setUint16(22, channels, true);
        view.setUint32(24, sampleRate, true);
        view.setUint32(28, sampleRate * channels * (bitsPerSample/8)); // Byte rate
        view.setUint16(32, channels * (bitsPerSample/8)); // Block align
        view.setUint16(34, bitsPerSample);
        writeString(view, 36, "data"); // SubChunk2id
        view.setUint32(40, subChunk2Size, true);
        
        const wavBytes = new Uint8Array(header.byteLength + pcmData.value.length)
        wavBytes.set(new Uint8Array(header), 0);
        wavBytes.set(pcmData, header.byteLength);

        const headers = new Headers({
            "Content-Type": "audio/wav",
            "Content-Disposition": `attachment; filename="result.wav"`
        })

        return new Response(
            wavBytes,
            {headers}
        )

    }
}