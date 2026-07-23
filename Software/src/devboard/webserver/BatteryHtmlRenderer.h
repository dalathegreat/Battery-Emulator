#ifndef _BATTERY_HTML_RENDERER_H
#define _BATTERY_HTML_RENDERER_H

#include "../../datalayer/datalayer.h"

#include <Arduino.h>
#include <WString.h>
#include <stdio.h>

// Each battery can implement this interface to render more battery specific HTML
// content
class BatteryHtmlRenderer {
 public:
  virtual String get_status_html() = 0;

  // Returns true when this renderer reads the data of its own battery instance
  // (e.g. via a datalayer pointer supplied at construction), rather than the
  // hardcoded main-battery globals. The advanced battery page only renders
  // status HTML for battery 2/3 when this returns true; otherwise it shows a
  // "limited to the Main Battery" notice. Override and return true after
  // reworking a battery integration for double/triple support.
  virtual bool renders_own_battery_data() { return false; }

  static String render_dtc_section_html(DATALAYER_BATTERY_DTC_TYPE& dtc, const char* json_filename,
                                        bool standard_code_string) {
    String content;

    // Reserve enough space to avoid reallocs
    content.reserve(3300 + dtc.dtc_count * 320);
    content +=
        "<h4 style='margin-top:20px;color:#27b06c;border-bottom:2px solid #27b06c;padding-bottom:5px;'>&#128295; "
        "Diagnostic Trouble Codes</h4>";
    if (dtc.dtc_last_read_millis == 0) {
      content += "<p style='color:#bbb;'>Not read yet &mdash; use the Read DTC button below to scan.</p>";
    } else if (dtc.dtc_read_failed) {
      content += "<p style='color:#ff8a80;'>&#9888; Last DTC read failed or timed out.</p>";
    } else if (dtc.dtc_count == 0) {
      content += "<p style='color:#69f0ae;'>&#10003; No DTCs present.</p>";
    } else {
      unsigned long age_s = (millis() - dtc.dtc_last_read_millis) / 1000;
      content +=
          "<p style='color:#bbb;'>" + String(dtc.dtc_count) + " codes &mdash; read " + String(age_s) + "s ago</p>";
      content += "<div style='overflow-x:auto;margin-bottom:12px;'>";
      content +=
          "<table style='margin:0 auto;text-align:left;border-collapse:separate;border-spacing:0;"
          "border:1px solid #4a5a64;border-radius:8px;overflow:hidden;'>";
      content +=
          "<thead><tr style='background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;'>"
          "<th style='padding:10px 18px;text-align:left;'>DTC</th>"
          "<th style='padding:10px 18px;text-align:left;'>Status</th>"
          "<th style='padding:10px 18px;text-align:left;'>Description</th></tr></thead><tbody>";

      const char SYS[5] = "PCBU";
      for (int i = 0; i < dtc.dtc_count; i++) {
        uint32_t code = dtc.dtc_codes[i];
        uint8_t status = dtc.dtc_status[i];

        char dtcStr[12];
        String matchKey;
        if (standard_code_string) {
          // SAE format (P0C9500, B1B4E12, etc.)
          snprintf(dtcStr, sizeof(dtcStr), "%c%02lX%02lX%02lX", SYS[(code >> 22) & 0x03],
                   (unsigned long)((code >> 16) & 0x3F), (unsigned long)((code >> 8) & 0xFF),
                   (unsigned long)(code & 0xFF));
          matchKey = String(dtcStr);
        } else {
          // Raw 6-digit hex code (9B4E12, DB4E12, etc.)
          snprintf(dtcStr, sizeof(dtcStr), "%06lX", (unsigned long)code);
          matchKey = String(code);
        }

        // Status precedence: Active (bit 0x01) > Confirmed (bit 0x08) > Stored.
        const char* statusStr = "Stored";
        const char* statusColor = "#9e9e9e";
        if (status & 0x08) {
          statusStr = "Confirmed";
          statusColor = "#d29922";
        }
        if (status & 0x01) {
          statusStr = "Active";
          statusColor = "#ff5252";
        }

        content +=
            "<tr><td style='padding:8px 18px;border-top:1px solid #3a4750;font-family:monospace;"
            "font-weight:600;'>";
        content += dtcStr;
        content +=
            "</td>"
            "<td style='padding:8px 18px;border-top:1px solid #3a4750;color:";
        content += statusColor;
        content += ";font-weight:600;'>";
        content += statusStr;
        content +=
            "</td>"
            "<td data-dtc-code='";
        content += matchKey;
        content += "' style='padding:8px 18px;border-top:1px solid #3a4750;'>Unknown</td></tr>";
      }
      content += "</tbody></table></div>";
      content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, json_filename);
    }

    return content;
  }

  // Base URL for the upstream GitHub repository's data folder.
  // Battery renderers can pass this to get_dtc_json_loader_html() directly,
  // or supply their own URL string for a different server/fork.
  static constexpr const char* GITHUB_RAW_BASE_URL =
      "https://raw.githubusercontent.com/dalathegreat/Battery-Emulator/main/web_data/dtc/";
  // Renders a status line + optional file-picker widget + JavaScript that fills
  // DTC descriptions into any table cell carrying a data-dtc-code attribute.
  // The attribute may hold either the decimal code (matched against the JSON
  // "code" field, e.g. data-dtc-code='41848') or the DTC string (matched against
  // the JSON "dtc" field, e.g. data-dtc-code='P0C9500'). JSON entries may carry
  // "code", "dtc", or both; the loader keys on whichever is present.
  //
  // base_url:  Base URL for the JSON file, e.g. GITHUB_RAW_BASE_URL.
  //             Pass "" (default) to skip the GitHub fetch.
  // filename:  Single JSON filename under base_url, e.g. "meb_dtc.json"
  //   On a successful GitHub fetch the file picker is hidden automatically.
  //   On failure the file picker is revealed so the user can load a local copy.
  static String get_dtc_json_loader_html(const char* base_url = "", const char* filename = "") {
    String s;
    s.reserve(4096);  // avoid repeated 16-byte realloc steps while building the loader
    s += "<div style='margin-top:15px;padding:12px;background:#1e1e2e;border:1px solid #444;border-radius:8px;'>";
    s += "<p id='dtcJsonStatus' style='margin:0 0 8px 0;color:#aaa;font-size:.95em;'></p>";
    s += "<div id='dtcJsonFileContainer' style='display:none;'>";
    s += "<p style='margin:4px 0;color:#ccc;font-size:.9em;'><strong>Load DTC descriptions from a local JSON "
         "file</strong> ";
    s += "(e.g. <em>";
    s += filename;
    s += "</em>):</p>";
    s += "<input type='file' id='dtcJsonFile' accept='.json' "
         "style='color:#ccc;background:#2a2a3e;border:1px solid #555;border-radius:4px;padding:4px "
         "8px;cursor:pointer;'>";
    s += "</div>";
    s += "</div>";

    // ---------------------------------------------------------------------------
    // DTC description loader script.
    //
    // The block emitted below is a MINIFIED version (short identifiers, no
    // whitespace) to save flash. Do NOT hand-edit the minified literal: edit the
    // readable source in the comment block, re-minify, and replace the literal.
    //
    // Identifier map (minified -> readable):
    //   u=url  k=cacheKey  S=statusEl  C=fileContainer  I=fileInput
    //   A=applyDtcs  P=showFilePicker  F=fetchFromGitHub
    //   a=arr b=fromCache m=map L=cells n=matched t=td/text e=entry d=desc-html
    //   g=cached f=file R=reader v=ev x=ex
    //
    // ORIGINAL (readable) JAVASCRIPT:
    /*
    <script>
    (function(){
      var url = base_url + filename;          // injected at build time
      var cacheKey = 'dtcJson:' + url;
      var statusEl = document.getElementById('dtcJsonStatus');
      var fileContainer = document.getElementById('dtcJsonFileContainer');
      var fileInput = document.getElementById('dtcJsonFile');

      function applyDtcs(arr, fromCache) {
        var map = {};
        // Key by decimal code and/or DTC string, whichever the entry has.
        arr.forEach(function(e){
          if (e.code != null) map[e.code] = e;
          if (e.dtc) map[e.dtc] = e;
        });
        var cells = document.querySelectorAll('[data-dtc-code]');
        var matched = 0;
        cells.forEach(function(td){
          var e = map[td.getAttribute('data-dtc-code')];
          if (e) {
            var html = e.l_dsc;
            if (e.s_dsc)
              html += '<br /><em style=\'color:#aaa;font-size:0.85em\'>' + e.s_dsc + '</em>';
            td.innerHTML = html;
            matched++;
          }
        });
        var src = fromCache ? ' (cached)' : ' (fetched)';
        statusEl.innerHTML = 'Loaded ' + arr.length + ' entries, ' + matched + '/' + cells.length +
          ' DTCs matched' + src +
          '. <a href=\'#\' id=\'dtcRefresh\' style=\'color:#aaa;font-size:0.85em;\'>Refresh</a>';
        statusEl.style.color = matched > 0 ? '#4CAF50' : '#ff9800';
        document.getElementById('dtcRefresh').addEventListener('click', function(e){
          e.preventDefault();
          try { localStorage.removeItem(cacheKey); } catch(ex) {}
          fetchFromGitHub();
        });
      }

      function showFilePicker(msg) {
        statusEl.textContent = msg;
        statusEl.style.color = '#ff9800';
        fileContainer.style.display = '';
      }

      function fetchFromGitHub() {
        statusEl.textContent = 'Fetching DTC descriptions from GitHub...';
        statusEl.style.color = '#aaa';
        fetch(url).then(function(r){
          if (!r.ok) throw new Error('HTTP ' + r.status);
          return r.text();
        }).then(function(text){
          try { localStorage.setItem(cacheKey, text); } catch(ex) {}
          applyDtcs(JSON.parse(text), false);
        }).catch(function(err){
          showFilePicker('GitHub unavailable (' + err.message + ') - load from local file:');
        });
      }

      if (url.length > 0) {
        var cached = null;
        try { cached = localStorage.getItem(cacheKey); } catch(ex) {}
        if (cached) {
          try { applyDtcs(JSON.parse(cached), true); } catch(ex) { fetchFromGitHub(); }
        } else {
          fetchFromGitHub();
        }
      } else {
        fileContainer.style.display = '';
      }

      fileInput.addEventListener('change', function(e){
        var file = e.target.files[0];
        if (!file) return;
        statusEl.textContent = 'Loading...';
        statusEl.style.color = '#aaa';
        var reader = new FileReader();
        reader.onload = function(ev){
          try { applyDtcs(JSON.parse(ev.target.result), false); }
          catch(err) {
            statusEl.textContent = 'Parse error: ' + err.message;
            statusEl.style.color = '#d32f2f';
          }
        };
        reader.onerror = function(){
          statusEl.textContent = 'File read error';
          statusEl.style.color = '#d32f2f';
        };
        reader.readAsText(file);
      });
    })();
    </script>
    */
    // ---------------------------------------------------------------------------
    s += "<script>(function(){var u='";
    s += base_url;
    s += filename;
    s += "';var k='dtcJson:'+u;"
         "var S=document.getElementById('dtcJsonStatus');"
         "var C=document.getElementById('dtcJsonFileContainer');"
         "var I=document.getElementById('dtcJsonFile');"
         "function A(a,b){"
         "var m={};a.forEach(function(e){if(e.code!=null)m[e.code]=e;if(e.dtc)m[e.dtc]=e;});"
         "var L=document.querySelectorAll('[data-dtc-code]');var n=0;"
         "L.forEach(function(t){var e=m[t.getAttribute('data-dtc-code')];"
         "if(e){var d=e.l_dsc;if(e.s_dsc)d+='<br /><em "
         "style=\\'color:#aaa;font-size:0.85em\\'>'+e.s_dsc+'</em>';t.innerHTML=d;n++;}});"
         "S.innerHTML='Loaded '+a.length+' entries, '+n+'/'+L.length+' DTCs matched'+(b?' (cached)':' (fetched)')+"
         "'. <a href=\\'#\\' id=\\'dtcRefresh\\' style=\\'color:#aaa;font-size:0.85em;\\'>Refresh</a>';"
         "S.style.color=n>0?'#4CAF50':'#ff9800';"
         "document.getElementById('dtcRefresh').addEventListener('click',function(e){"
         "e.preventDefault();try{localStorage.removeItem(k);}catch(x){}F();});}"
         "function P(msg){S.textContent=msg;S.style.color='#ff9800';C.style.display='';}"
         "function F(){S.textContent='Fetching DTC descriptions from GitHub...';S.style.color='#aaa';"
         "fetch(u).then(function(r){if(!r.ok)throw new Error('HTTP '+r.status);return r.text();})"
         ".then(function(t){try{localStorage.setItem(k,t);}catch(x){}A(JSON.parse(t),false);})"
         ".catch(function(err){P('GitHub unavailable ('+err.message+') - load from local file:');});}"
         "if(u.length>0){var g=null;try{g=localStorage.getItem(k);}catch(x){}"
         "if(g){try{A(JSON.parse(g),true);}catch(x){F();}}else{F();}}else{C.style.display='';}"
         "I.addEventListener('change',function(e){var f=e.target.files[0];if(!f)return;"
         "S.textContent='Loading...';S.style.color='#aaa';var R=new FileReader();"
         "R.onload=function(v){try{A(JSON.parse(v.target.result),false);}"
         "catch(err){S.textContent='Parse error: '+err.message;S.style.color='#d32f2f';}};"
         "R.onerror=function(){S.textContent='File read error';S.style.color='#d32f2f';};"
         "R.readAsText(f);});})();</script>";
    return s;
  }
};

class BatteryDefaultRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() { return String("No extra information available for this battery type"); }
  // Static text, valid for any battery instance.
  bool renders_own_battery_data() { return true; }
};

#endif
