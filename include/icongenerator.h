#ifndef ICONGENERATOR_H
#define ICONGENERATOR_H

#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/colour.h>
#include <vector>

class IconGenerator
{
public:
    static wxBitmap generateDefaultIcon(const wxString& name, int size = 64);
    static wxBitmap generateIcon(const wxString& text, const wxColour& backgroundColor,
                                  const wxColour& textColor = *wxWHITE, int size = 64);
    static const std::vector<wxColour>& getDefaultColors();
    static wxColour getColorForName(const wxString& name);

private:
    static wxBitmap drawCircularIcon(const wxString& text, const wxColour& backgroundColor,
                                      const wxColour& textColor, int size);
};

#endif // ICONGENERATOR_H