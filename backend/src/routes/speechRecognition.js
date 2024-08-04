import changeToWav from "../lib/changeToWav";

/**
 * 
 * @param {Request} request
 * @param {any} ctx
 */
export default async function (request, ctx) {
    
    /** @type {ReadableStream<Uint8Array>} */
    const requestBody = request.body;
    if (requestBody == null) {
        return new Response.error();
    }

    // Run speech recognition

    const wavBytes = changeToWav(await request.arrayBuffer());

    const sttResponse = await ctx.env.AI.run(
        "@cf/openai/whisper",
        {
            audio: [...wavBytes]
        }
    )

    return Response.json({
        text: sttResponse.text
    })

}