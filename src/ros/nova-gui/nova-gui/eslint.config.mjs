// @ts-check

import eslint from '@eslint/js';
import tseslint from 'typescript-eslint';
import reactPlugin from "eslint-plugin-react";

export default tseslint.config({
    extends: [
        eslint.configs.recommended,
        tseslint.configs.recommended,
    ],
    plugins: {
        "@eslint-plugin-react": reactPlugin,
    },
    rules: {
        '@typescript-eslint/no-duplicate-enum-values': 'warn',
        '@typescript-eslint/no-empty-object-type': 'off',
        "@typescript-eslint/no-unused-vars": [
            "error",
            {
                "args": "all",
                "argsIgnorePattern": "^_",
                "caughtErrors": "all",
                "caughtErrorsIgnorePattern": "^_",
                "destructuredArrayIgnorePattern": "^_",
                "varsIgnorePattern": "^_",
                "ignoreRestSiblings": true
            }
        ]
    },
});
