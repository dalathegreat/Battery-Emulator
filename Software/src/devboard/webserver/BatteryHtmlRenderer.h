#ifndef _BATTERY_HTML_RENDERER_H
#define _BATTERY_HTML_RENDERER_H

#include <WString.h>

// Each battery can implement this interface to render more battery specific HTML
// content
class BatteryHtmlRenderer {
 public:
  virtual String get_status_html() = 0;

  // Base URL for the upstream GitHub repository's data folder.
  // Battery renderers can pass this to get_dtc_json_loader_html() directly,
  // or supply their own URL string for a different server/fork.
  static constexpr const char* GITHUB_RAW_BASE_URL =
      "https://raw.githubusercontent.com/dalathegreat/Battery-Emulator/main/dtc_files/";
  // Renders a status line + optional file-picker widget + JavaScript that fills
  // DTC descriptions into any table cell carrying a data-dtc-code='<decimal>'
  // attribute.
  //
  // base_url:  Base URL for the JSON file, e.g. GITHUB_RAW_BASE_URL.
  //             Pass "" (default) to skip the GitHub fetch.
  // filename:  Single JSON filename under base_url, e.g. "meb_dtc_003.json"
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
    //   a=arr b=fromCache m=map L=cells n=matched t=td/text c=code
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
        arr.forEach(function(e){ map[e.code] = e; });
        var cells = document.querySelectorAll('[data-dtc-code]');
        var matched = 0;
        cells.forEach(function(td){
          var code = parseInt(td.getAttribute('data-dtc-code'), 10);
          if (map[code]) {
            td.innerHTML = map[code].l_dsc +
              '<br /><em style=\'color:#aaa;font-size:0.85em\'>' + map[code].s_dsc + '</em>';
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
         "var m={};a.forEach(function(e){m[e.code]=e;});"
         "var L=document.querySelectorAll('[data-dtc-code]');var n=0;"
         "L.forEach(function(t){var c=parseInt(t.getAttribute('data-dtc-code'),10);"
         "if(m[c]){t.innerHTML=m[c].l_dsc+'<br /><em "
         "style=\\'color:#aaa;font-size:0.85em\\'>'+m[c].s_dsc+'</em>';n++;}});"
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
};

#endif
