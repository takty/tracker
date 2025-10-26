/**
 *
 * Reader and Writer of Text Files
 *
 * @author Takuto Yanagida
 * @version 2025-10-26
 *
 */


#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "file_utils.hpp"


class TextReaderWriter {

public:

	static std::vector<std::wstring> read(const std::wstring& path) {
		std::vector<std::wstring> lines{};
		std::ifstream ifs(path, std::ios::binary);
		if (!ifs) return lines;

		ifs.seekg(0, std::ios::end);
		const std::streamoff total = ifs.tellg();
		if (total <= 0) return lines;

		ifs.seekg(0, std::ios::beg);
		unsigned char bom[2]{};
		bool hasBOM = false;
		if (total >= 2) {
			ifs.read(reinterpret_cast<char*>(bom), 2);
			hasBOM = (bom[0] == 0xFF && bom[1] == 0xFE);
		}

		const std::streamoff start = hasBOM ? 2 : 0;
		const std::streamoff remain = total - start;
		ifs.seekg(start, std::ios::beg);

		if (remain <= 0 || (remain % 2) != 0) return lines;

		std::wstring text;
		text.resize(static_cast<size_t>(remain / 2));
		if (!ifs.read(reinterpret_cast<char*>(text.data()), remain)) {
			const auto got = static_cast<size_t>(ifs.gcount());
			if (got % 2 == 0) {
				text.resize(got / 2);
			} else {
				return lines;
			}
		}

		size_t pos = 0;
		const size_t n = text.size();
		while (pos < n) {
			const size_t nl = text.find_first_of(L"\r\n", pos);
			if (nl == std::wstring::npos) {
				lines.emplace_back(text.substr(pos));
				break;
			}
			lines.emplace_back(text.substr(pos, nl - pos));
			if (text[nl] == L'\r' && nl + 1 < n && text[nl + 1] == L'\n') {
				pos = nl + 2;
			} else {
				pos = nl + 1;
			}
		}
		return lines;
	}

	static void write(const std::wstring& path, const std::vector<std::wstring>& lines) {
		std::ofstream ofs(path, std::ios::binary);
		if (!ofs) return;

		const unsigned char bom[2]{0xFF, 0xFE};
		ofs.write(reinterpret_cast<const char*>(bom), sizeof(bom));

		for (const auto& line : lines) {
			ofs.write(reinterpret_cast<const char*>(line.data()), static_cast<std::streamsize>(line.size() * sizeof(wchar_t)));

			const wchar_t crlf[] = L"\r\n";
			ofs.write(reinterpret_cast<const char*>(crlf), static_cast<std::streamsize>(2 * sizeof(wchar_t)));
		}
		ofs.close();
	}

};
