import changeToWav from "../lib/changeToWav";

/**
 * 
 * @param {Request} request
 * @param {any} ctx
 * @returns {Response}
 */

export default async function (request, ctx) {
    
    /** @type {ReadableStream<Uint8Array>} */
    const requestBody = request.body;
    if (requestBody == null) {
        return new Response.error();
    }

    const wavBytes = changeToWav(await request.arrayBuffer());

    const headers = new Headers({
        "Content-Type": "audio/wav",
        "Content-Disposition": `attachment; filename="result.wav"`
    })

    return new Response(
        wavBytes,
        {headers}
    )

}