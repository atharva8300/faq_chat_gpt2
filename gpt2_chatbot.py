import sys
from transformers import GPT2LMHeadModel, GPT2Tokenizer

def get_response(message):
    tokenizer = GPT2Tokenizer.from_pretrained('gpt2-large')
    model = GPT2LMHeadModel.from_pretrained('gpt2-large')

    inputs = tokenizer.encode(message, return_tensors='pt')

    outputs = model.generate(
        inputs,
        max_length=100,
        num_return_sequences=1,
        pad_token_id=tokenizer.eos_token_id,
        do_sample=True,  # Enable sampling to generate diverse results
        temperature=0.9,  # Adjust temperature for creativity, higher is more creative
        top_p=0.92,  # Nucleus sampling: higher results in more diversity
        top_k=50  # Top-k sampling: limits the number of high probability tokens considered
    )

    return tokenizer.decode(outputs[0], skip_special_tokens=True)

if __name__ == "__main__":
    message = sys.argv[1] if len(sys.argv) > 1 else "Hello"
    response = get_response(message)
    print(response.strip())
