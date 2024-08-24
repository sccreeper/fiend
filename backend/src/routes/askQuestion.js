const systemPrompt = `You are fiend, and are supposed to act as somebody's friend. You are a parody of another AI project called "friend". You should provide short responses, at most 2-3 sentences in length and under 500 characters. Do not use any emojis.`

const validChars = ` !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\`abcdefghijklmnopqrstuvwxyz{|}~`

/**
 * 
 * @param {Request} request
 * @param {any} ctx
 * @returns {Promise<Response>}
 */
export default async function (request, ctx) {

    const form = await request.formData();
    const text = form.get("question");

    // Run LLM response

    const messages = [
        {
            role: "system",
            content: systemPrompt
        },
        {
            role: "user",
            content: text
        }
    ]

    const llmResponse = await ctx.env.AI.run(
        "@cf/meta/llama-2-7b-chat-fp16",
        {messages}
    )

    let sanitizedResponse = "";

    for (let i = 0; i < llmResponse.response.length; i++) {
        const char = llmResponse.response[i];

        if (validChars.includes(char)) {
            sanitizedResponse+=char;
        }
        
    }

    return Response.json({
        userMessage: text,
        llmResponse: sanitizedResponse
    });
 
}