## Configuration

To use, rename the configuration file to `config.json` and place it in the same directory as the main program.
This directory already contains two preset configuration files: one for the LLaMA 3 series models (for multilingual
support) and one for the Sakura LLM series models (for Japanese-Chinese translation). Use them as needed.

If you need to fine-tune the configuration file, you can refer to the following instructions:

```json
{
  "vad": {
    // Can use "ten_vad" or "silero_vad"
    "silero_vad": {
      "model": "./models/vad/silero_vad.onnx",
      // Minimum silence duration in seconds. Use 0.02-0.05 for fast speech, 0.1-0.5 for slow speech.
      "min_silence_duration": 0.02,
      // Minimum speech duration in seconds. Use 0.8 for fast speech, and increase to 2.0-3.0 for slower speech.
      "min_speech_duration": 0.8,
      // Maximum speech duration in seconds. For fast speech, 2.0 is recommended to reduce latency. If speech is not fast, it can be extended to 3.0-10.0.
      "max_speech_duration": 2.0,
      // Model threshold. The lower the value, the easier speech is detected, but noise is also more likely to be treated as speech.
      "threshold": 0.3
    },
    "num_threads": 2
  },
  "asr": {
    // Can use "dolphin" or "sense_voice"
    "sense_voice": {
      "model_path": "./models/sense-voice-zh-en-ja-ko-yue-2024-07-17/model.int8.onnx",
      "token_path": "./models/sense-voice-zh-en-ja-ko-yue-2024-07-17/tokens.txt",
      // Can be set to "ja" to improve accuracy in Japanese-only scenarios. Chinese, English, Korean, and Cantonese are "zh", "en", "ko", and "yue" respectively.
      "language": "auto"
    },
    "num_threads": 2,
    // Uses CPU by default, can be configured to "directml" to activate GPU acceleration.
    "provider": "cpu"
  },
  "llm": {
    // OpenAI standard-compliant API, no need to end with /v1. Must support calls via `/v1/completions`.
    "api_base": "https://OPENAI-LIKE-API-BASE",
    // Authentication Token
    "api_token": "sk-aXTdpyG7c6G51cjj241b17Ea38F146A88a92513c89C22e77",
    // Model name
    "model_name": "llama-3.3-70b-inst",
    // When using Sakura LLM, please change to this
    "prompt_template": "<|start_header_id|>user<|end_header_id|>\n\nTreat next line as plain text input and translate it into Chinese, output translation ONLY. If translation is unnecessary (e.g. proper nouns, codes, etc.), return the original text. NO explanations. NO notes. Input:%TEXT%<|eot_id|><|start_header_id|>assistant<|end_header_id|>\n",
    // Maximum length of the model's output
    "max_tokens": 512,
    // Model temperature. The higher the value, the more random the output.
    "temperature": 0.1,
    // The proportion of candidates to consider for sampling
    "top_p": 0.3,
    // The current return value of the LLaMA.cpp API is slightly different from the standard OpenAI API. Set to true to adapt to the difference.
    "is_llama_cpp": false,
    // This option needs to be enabled when using Sakura LLM to prevent the model from generating indefinitely.
    "is_sakura_llm": false
  }
}
```