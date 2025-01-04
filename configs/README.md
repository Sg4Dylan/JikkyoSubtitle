## 配置

使用时将配置文件改名为 config.json，放在与主程序相同目录下即可。  
本目录中已经包含两个预设配置文件，适用于 LLaMA 3 系列模型（多语言支持）及 Sakura LLM 系列模型（日中翻译使用），可按需使用。  

如需对配置文件微调，可以参考以下说明：
```json
{
  "vad": {
    "silero_vad": {
      "model": "./models/vad/silero_vad.onnx",
      // 最短停顿间隔时间，单位为秒，语速较快时使用 0.02~0.05，语速较慢时使用 0.1~0.5
      "min_silence_duration": 0.02,
      // 最短语音持续时间，单位为秒，语速较快时使用 0.8，较慢时适当提高到 2.0~3.0
      "min_speech_duration": 0.8,
      // 最长语音持续时间，单位为秒，语速较快时建议使用 2.0 来减少延迟，语速不快可以适当加长到 3.0~10.0
      "max_speech_duration": 2.0,
      // 模型阈值，越低则语音越容易被检出，同时噪声更容易被当作语音
      "threshold": 0.3
    },
    // 超过最长语音持续时间后，打断模型循环的概率，越低则句子越完整，越高则前后词语丢失越严重
    "interrupt_threshold": 0.998,
    "num_threads": 2
  },
  "asr": {
    "sense_voice": {
      "model_path": "./models/sense-voice-zh-en-ja-ko-yue-2024-07-17/model.int8.onnx",
      // 可以设置为 ja 来提高纯日语场景的准确率，中英韩粤分别为 zh、en、ko、yue
      "language": "auto",
      "num_threads": 1
    },
    "num_threads": 2,
    "provider": "cpu",
    "token_path": "./models/sense-voice-zh-en-ja-ko-yue-2024-07-17/tokens.txt"
  },
  "llm": {
    // 使用 Sakura LLM 请换成 
    "prompt_template": "<|start_header_id|>user<|end_header_id|>\n\nTreat next line as plain text input and translate it into Chinese, output translation ONLY. If translation is unnecessary (e.g. proper nouns, codes, etc.), return the original text. NO explanations. NO notes. Input:%TEXT%<|eot_id|><|start_header_id|>assistant<|end_header_id|>\n",
    // 模型名字
    "model_name": "llama-3.3-70b-inst",
    // 鉴权 Token
    "auth_key": "sk-aXTdpyG7c6G51cjj241b17Ea38F146A88a92513c89C22e77",
    // 符合 OpenAI 标准的接口，需要支持通过 `/v1/completions` 调用
    "api_base": "https://OPENAI-LIKE-API-BASE/",
    // 模型输出的最大长度
    "max_tokens": 512,
    // 模型温度，越高则输出越随机
    "temperature": 0.1,
    // 输出采样候选比例
    "top_p": 0.3,
    // 目前 LLaMA.cpp 提供的接口返回值与标准 OpenAI 接口略有不同，需要设置为 true 来适应差异
    "is_llama_cpp": false,
    // 使用 Sakura LLM 时需要开启这个选项避免模型停不下来
    "is_sakura_llm": false
  }
}
```
