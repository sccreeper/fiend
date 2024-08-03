import changeToWav from "../lib/changeToWav";

const systemPrompt = `You are fiend, and are supposed to act as somebody's friend. You are a parody of another AI project called "friend". You should provide short responses.`

/**
 * 
 * @param {Request} request
 * @param {any} ctx
 * @returns {Promise<Response>}
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

    // Run LLM response

    const messages = [
        {
            role: "system",
            content: systemPrompt
        },
        {
            role: "user",
            content: sttResponse.text
        }
    ]

    const llmResponse = await ctx.env.AI.run(
        "@cf/meta/llama-2-7b-chat-fp16",
        {messages}
    )

    return Response.json({
        userMessage: sttResponse.text,
        llmResponse: llmResponse.response
    });
 
}