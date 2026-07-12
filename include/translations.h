#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include <wx/intl.h>
#include <wx/translation.h>
#include <wx/string.h>
#include <wx/buffer.h>
#include <vector>
#include <utility>

// ============================================================================
// Embedded translations for WiseExec.
// Builds GNU gettext .mo binary data in memory, no external tools needed.
// ============================================================================

class EmbeddedTranslationsLoader : public wxTranslationsLoader
{
public:
    using StringPair = std::pair<wxString, wxString>; // msgid, msgstr

    void AddStrings(const wxString& lang, std::vector<StringPair> strings)
    {
        m_strings[lang] = std::move(strings);
    }

    wxMsgCatalog* LoadCatalog(const wxString& domain,
                              const wxString& lang) override
    {
        auto it = m_strings.find(lang);
        if (it == m_strings.end())
            return nullptr;

        wxScopedCharBuffer mo = BuildMoData(it->second);
        return wxMsgCatalog::CreateFromData(mo, domain);
    }

    wxArrayString GetAvailableTranslations(const wxString& domain) const override
    {
        wxArrayString langs;
        if (domain == wxT("WiseExec")) {
            for (auto& kv : m_strings)
                langs.Add(kv.first);
        }
        return langs;
    }

private:
    // Build a valid GNU gettext .mo file in memory.
    // .mo layout:
    //   header (28 bytes)
    //   orig string table  (N × 8 bytes)
    //   trans string table (N × 8 bytes)
    //   string data (all strings packed)
    static wxScopedCharBuffer BuildMoData(const std::vector<StringPair>& pairs)
    {
        // Prepend the mandatory metadata header entry (msgid="" with charset).
        // GNU gettext requires the first entry to be metadata; without it,
        // wxMsgCatalogFile::LoadData() won't detect the charset and FillHash()
        // falls back to wxConvCurrent, which may not handle UTF-8 on Windows.
        std::vector<StringPair> all;
        all.reserve(pairs.size() + 1);
        all.emplace_back(wxT(""),
            wxT("Content-Type: text/plain; charset=UTF-8\n"
                "Plural-Forms: nplurals=1; plural=0;\n"));
        all.insert(all.end(), pairs.begin(), pairs.end());

        const uint32_t n = static_cast<uint32_t>(all.size());

        // Collect all strings: first orig, then trans
        // Offsets start after header + two tables
        const uint32_t headerSize = 28;
        const uint32_t tableSize = n * 8;
        uint32_t offset = headerSize + tableSize * 2; // orig table + trans table

        struct Entry { uint32_t length; uint32_t offset; };
        std::vector<Entry> origTable(n), transTable(n);
        std::vector<char> stringData;

        for (uint32_t i = 0; i < n; ++i) {
            // Original string
            wxCharBuffer orig = all[i].first.ToUTF8();
            uint32_t origLen = static_cast<uint32_t>(strlen(orig.data()));
            origTable[i] = { origLen, offset };
            stringData.insert(stringData.end(), orig.data(), orig.data() + origLen);
            stringData.push_back('\0');
            offset += origLen + 1;

            // Translated string
            wxCharBuffer trans = all[i].second.ToUTF8();
            uint32_t transLen = static_cast<uint32_t>(strlen(trans.data()));
            transTable[i] = { transLen, offset };
            stringData.insert(stringData.end(), trans.data(), trans.data() + transLen);
            stringData.push_back('\0');
            offset += transLen + 1;
        }

        // Build the complete .mo binary blob
        std::vector<char> mo(headerSize + tableSize * 2 + stringData.size());
        char* p = mo.data();

        auto write32 = [&](uint32_t v) {
            memcpy(p, &v, 4); p += 4;
        };

        // Header
        write32(0x950412de); // magic
        write32(0);          // revision
        write32(n);          // nstrings
        write32(headerSize);                 // orig_table_offset
        write32(headerSize + tableSize);     // trans_table_offset
        write32(0);          // hash_size
        write32(0);          // hash_offset

        // Orig table
        for (uint32_t i = 0; i < n; ++i) {
            write32(origTable[i].length);
            write32(origTable[i].offset);
        }
        // Trans table
        for (uint32_t i = 0; i < n; ++i) {
            write32(transTable[i].length);
            write32(transTable[i].offset);
        }
        // String data
        memcpy(p, stringData.data(), stringData.size());

        wxCharBuffer buf(mo.size());
        memcpy(buf.data(), mo.data(), mo.size());
        return buf;
    }

    std::unordered_map<wxString, std::vector<StringPair>> m_strings;
};

#endif // TRANSLATIONS_H
