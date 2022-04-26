const String re_logo_smol_base64 = "<img src=\"data:image/png;base64,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\" />";


