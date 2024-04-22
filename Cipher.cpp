#include <string>
#include<iostream>

std::string encrypt(std::string text, int key) {
    std::string result = "";

    for (int i = 0; i < text.length(); i++) {
        if (isupper(text[i]))
            result += char(int(text[i] + key - 65) % 26 + 65);
        else
            result += char(int(text[i] + key - 97) % 26 + 97);
    }

    return result;
}

std::string decrypt(std::string cipherText, int key) {
    std::string result = "";

    for (int i = 0; i < cipherText.length(); i++) {
        if (isupper(cipherText[i]))
            result += char(int(cipherText[i] - key - 65 + 26) % 26 + 65);
        else
            result += char(int(cipherText[i] - key - 97 + 26) % 26 + 97);
    }

    return result;
}

int main() {
    std::string text = "HELLO";
    int key = 3;
    std::string cipherText = encrypt(text, key);
    std::string originalText = decrypt(cipherText, key);

    std::cout << "Original Text: " << text << std::endl;
    std::cout << "Encrypted Text: " << cipherText << std::endl;
    std::cout << "Decrypted Text: " << originalText << std::endl;

    return 0;
}
