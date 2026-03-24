#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#define INDEX_HTML_HEADER \
  R"rawliteral(<!doctype html><html>
  <head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Battery Emulator</title>
  <link rel="icon" type="image/svg+xml" href="data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxMDAgMTAwIj48dGV4dCB5PSIuOWVtIiBmb250LXNpemU9IjkwIj7imqE8L3RleHQ+PC9zdmc+">
  <style>
      /* --- 🎨 1. Base Styles (CSS) --- */
      body { 
        margin: 0; 
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
        background-color: #f4f4f4; 
        color: #333; 
        display: flex; 
        flex-direction: column; 
        height: 100vh; 
        overflow: hidden; 
      }
      
      /* --- 🔝 Top Header --- */
      .header { 
        background-color: #1a1a1a; 
        color: #fff; 
        padding: 15px 25px; 
        display: flex; 
        justify-content: space-between; 
        align-items: center; 
        box-shadow: 0 2px 5px rgba(0,0,0,0.2); 
        z-index: 10; 
      }
      .header-left { display: flex; align-items: center; }
      .header h1 { margin: 0; font-size: 1.3rem; }
      .fw-version { font-size: 0.8rem; color: #aaa; margin-left: 10px; font-weight: normal; }
      .header-status { font-size: 0.9rem; }
      
      /* --- 📐 Layout Structure --- */
      .container { display: flex; flex: 1; overflow: hidden; position: relative; }

      /* --- 📑 Sidebar Navigation --- */    
      .sidebar, aside, #sidebar, .main-sidebar { 
        width: 260px; 
        padding-bottom: 90px !important; 
        background-color: #ffffff; 
        border-right: 1px solid #ddd; 
        overflow-y: auto !important;     
        display: flex; 
        flex-direction: column; 
        transition: left 0.3s ease-in-out;
      -webkit-overflow-scrolling: touch; 
      }
      
      .menu-group { 
        background-color: #333; 
        color: #fff; 
        padding: 10px 15px; 
        font-size: 0.85rem; 
        font-weight: bold; 
        text-transform: uppercase; 
        margin-top: 15px; 
        letter-spacing: 1px; 
      }
      .menu-group:first-child { margin-top: 0; }
      
      .menu-item { 
        padding: 12px 20px; 
        text-decoration: none; 
        color: #555; 
        border-bottom: 1px solid #f0f0f0; 
        display: flex; 
        align-items: center; 
        justify-content: space-between; 
        font-size: 0.95rem; 
        cursor: pointer; 
        transition: background 0.2s; 
      }
      .menu-item:hover { background-color: #f9f9f9; color: #000; }
      .menu-item.active { font-weight: bold; color: #000; background-color: #e9ecef; border-left: 4px solid #e74c3c; }
      
      /* --- 🔴 Notification Badge --- */
      .badge { 
        background-color: #e74c3c; 
        color: white; 
        padding: 2px 8px; 
        border-radius: 12px; 
        font-size: 0.75rem; 
        font-weight: bold; 
        display: none; /* Hidden by default */
      } 
      .badge.show { display: inline-block; }

      /* --- 📄 Content Area --- */
      .content { 
        flex: 1; 
        padding: 25px; 
        overflow-y: auto; 
        background-color: #f4f4f4; 
        box-sizing: border-box;
      }
      
      /* 🌟 SPA Core: Hide/Show pages */
      .page { display: none; animation: fadeIn 0.3s; }
      .page.active { display: block; }
      @keyframes fadeIn { from { opacity: 0; transform: translateY(5px); } to { opacity: 1; transform: translateY(0); } }

      /* --- 🗂️ UI Components (Cards & Buttons) --- */
      .card { background: #fff; padding: 20px; border-radius: 6px; box-shadow: 0 2px 5px rgba(0,0,0,0.05); margin-bottom: 20px; border-top: 4px solid #3498db; }
      .card-danger { border-top-color: #e74c3c; }
      .card-warning { border-top-color: #f39c12; }

      .btn { padding: 10px 15px; border: none; cursor: pointer; border-radius: 4px; font-weight: bold; font-size: 0.9rem; transition: 0.2s; }
      .btn-primary { background-color: #3498db; color: white; }
      .btn-danger { background-color: #e74c3c; color: white; }
      .btn-warning { background-color: #f39c12; color: white; }
      .btn:hover { filter: brightness(0.9); }

      /* --- 📱 Responsive Design (Mobile Support) --- */
      /* Hamburger Menu Button (Hidden on Desktop) */
      .menu-toggle { 
        display: none; 
        background: none; 
        border: none; 
        color: white; 
        font-size: 1.5rem; 
        margin-right: 15px; 
        cursor: pointer; 
        padding: 0; 
      }
      
      /* Adjustments for screens smaller than 768px (Mobile & Tablet) */
      @media (max-width: 768px) {
        .menu-toggle { display: block; } /* Show hamburger button */
        .header h1 { font-size: 1.1rem; }
        .header-status { font-size: 0.8rem; }
        
        .sidebar { 
          position: absolute; 
          left: -260px; /* Hide sidebar off-screen to the left */
          top: 0; 
          height: 100%; 
          z-index: 100; 
          box-shadow: 2px 0 10px rgba(0,0,0,0.5); 
        }
        
        /* Class toggled by JavaScript to slide menu in */
        .sidebar.open { left: 0; } 
        
        .content { padding: 15px; width: 100vw; }
      }
    </style>
    </head>
    <body>
      <div class="header">
          <div style="display: flex; align-items: center;">
            <button class="menu-toggle" onclick="toggleSidebar()">☰</button>
            <div style="font-size: 1.2rem; font-weight: bold;">⚡ Battery Emulator</div>
          </div>
          <div class="header-status">Status: <span id="topbar_status" style="font-weight: bold; color: #888;">--</span></div>
          
        </div>
        <div class="container">
          
          <div class="sidebar" id="sidebar">
            <div class="menu-group">Main Menu</div>
            <a href="/" class="menu-item">📊 Dashboard</a>
            <a href="/cellmonitor" class="menu-item">🔋 Cell Monitor</a>
            <a href="/advanced" class="menu-item">🛰️ Advanced</a>
            
            <div class="menu-group">Diagnostics</div>
            <a href="/events" class="menu-item">⚠️ Events <span class="badge" id="event-badge">0</span></a>
            <a href="/canlog" class="menu-item">🔢 CAN Log</a>
            <a href="/canreplay" class="menu-item">⏪ CAN Replay</a>
            <a href="/log" class="menu-item">📄 System Log</a>
            
            <div class="menu-group">Setup </div>
            <a href="/settings" class="menu-item">🛠️ Settings</a>
            
           <div class="menu-group">Administration</div>
            <a href="/ota" class="menu-item">🧬 OTA Update</a>
            <a href="#" class="menu-item" style="color: #e74c3c;" onclick="askReboot()">🔄 Reboot</a>
            <a href="#" class="menu-item" onclick="logout()"><span>🚪 Logout</span></a>
          </div>
          
        <div class="content">
)rawliteral"

#define INDEX_HTML_FOOTER \
  R"rawliteral(
    </div> </div> 
    <script>
      // JS auto highlight ative url (Red color)
      document.querySelectorAll('.menu-item').forEach(link => {
        if(window.location.href.endsWith(link.getAttribute('href'))) {
          link.classList.add('active');
        }
      });

      // Show/hidden for mobile support
      function toggleSidebar() {
        document.getElementById('sidebar').classList.toggle('open');
      }

      // 🔄 Pull-to-Refresh simulation script for Web App
      var touchStartY = 0;
      
      document.addEventListener('touchstart', function(e) {
        var scrollTop = window.scrollY || document.documentElement.scrollTop;
        var scrollBox = document.querySelector('.card-warning') || document.body; 
        if (scrollTop <= 0 && scrollBox.getBoundingClientRect().top >= 0) {
          touchStartY = e.touches[0].clientY;
        } else {
          touchStartY = 0; 
        }
      }, {passive: true});

      document.addEventListener('touchstart', function(e) {
        //🛑 Extra precaution: If your finger is hovering over the Terminal box or the left-hand menu, cancel the refresh immediately!
        if (e.target.closest('.terminal') || e.target.closest('.sidebar')) {
          touchStartY = 0;
          return; 
        }

        var scrollTop = window.scrollY || document.documentElement.scrollTop;
        var scrollBox = document.querySelector('.card-warning') || document.body; 
        if (scrollTop <= 0 && scrollBox.getBoundingClientRect().top >= 0) {
          touchStartY = e.touches[0].clientY;
        } else {
          touchStartY = 0; 
        }
      }, {passive: true});

      // Update status bar every 2s
      function updateTopbarStatus() {
        fetch('/api/data')
          .then(response => response.text())
          .then(text => {
            let data = window.repairAndParseJSON(text);
            if (data) {
              var topStatus = document.getElementById('topbar_status');
              if(topStatus) {
                topStatus.innerText = data.sys.status;
                if (data.sys.status === 'RUNNING') {
                  topStatus.style.color = '#2ecc71'; 
                } else if (data.sys.status.includes('PAUSE')) {
                  topStatus.style.color = '#f39c12'; 
                } else {
                  topStatus.style.color = '#e74c3c'; 
                }
              }
            }
          })
          .catch(error => console.log('Topbar sync error:', error));
      }
      
      updateTopbarStatus();
      setInterval(updateTopbarStatus, 2000);

      document.addEventListener('click', function(event) {
          if (window.innerWidth <= 768) {
              var sidebar = document.getElementById('sidebar');
              var isClickInsideSidebar = sidebar ? sidebar.contains(event.target) : false;
              var isClickHamburger = event.target.closest('button.menu-toggle');

              if (!isClickInsideSidebar && !isClickHamburger && sidebar) {
                  if (sidebar.classList.contains('open')) {
                      sidebar.classList.remove('open');
                  }
              }
          }
      });
    
      function logout() {
        if(confirm("Are you sure you want to log out?")) {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/logout", true, "logout", "logout");
          xhr.send();
          setTimeout(function() { window.location.href = "/logout"; }, 500);
        }
      }

      // 2. Checks the status and hides the button.
      window.addEventListener('DOMContentLoaded', function() {
        fetch('/api/is_auth').then(r => r.text()).then(status => {
          if(status.trim() === '0') {
            document.querySelectorAll('a').forEach(function(el) {
              if(el.getAttribute('onclick') && el.getAttribute('onclick').includes('logout(')) {
                el.style.display = 'none';
              }
            });
          }
        }).catch(e => console.log('Auth check error:', e));
      });
            
      function askReboot() {
        if (window.confirm('Are you sure you want to reboot the system? NOTE: If emulator is handling contactors, they will open during reboot!')) {
          var modal = document.createElement('div');
          modal.innerHTML = '<div style="position:fixed;top:0;left:0;width:100vw;height:100vh;background:rgba(0,0,0,0.85);z-index:99999;display:flex;justify-content:center;align-items:center;backdrop-filter:blur(5px);">' +
            '<div style="background:#fff;padding:35px 20px;border-radius:15px;text-align:center;box-shadow:0 10px 30px rgba(0,0,0,0.5);width:350px;max-width:90vw;font-family:sans-serif;">' +
              '<h2 style="color:#e74c3c;margin-top:0;font-size:1.8rem;">🔄 System Rebooting</h2>' +
              '<div style="border:5px solid #f3f3f3;border-top:5px solid #e74c3c;border-radius:50vw;width:60px;height:60px;animation:spin 1s linear infinite;margin:25px auto;"></div>' +
              '<p style="color:#555;font-size:1.1rem;margin:10px 0;">Please wait <span id="rebootCount" style="font-weight:bold;color:#e74c3c;font-size:1.3rem;">15</span> seconds.</p>' +
              '<p style="font-size:0.85rem;color:#888;">The system is restarting.<br>The page will reload automatically.</p>' +
            '</div></div><style>@keyframes spin { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }</style>';
          
          document.body.appendChild(modal);
          fetch('/reboot', { method: 'GET', keepalive: true }).catch(function(){});
          
          let timeLeft = 15;
          let countEl = document.getElementById('rebootCount');
          let timer = setInterval(function() {
            timeLeft--;
            if(countEl) countEl.innerText = timeLeft;
            if (timeLeft <= 0) {
              clearInterval(timer);
              window.location.href = '/';
            }
          }, 1000);
        }
      }

      window.repairAndParseJSON = function(jsonString) {
        if (!jsonString) return null;
          try {
            return JSON.parse(jsonString);
          } catch (e) {
            console.warn("⚠️ Data truncated! Attempting Auto-Heal...");
            let repaired = jsonString.trim().replace(/,\s*$/, '');
            
            let openBraces = (repaired.match(/\{/g) || []).length;
            let closeBraces = (repaired.match(/\}/g) || []).length;
            let openBrackets = (repaired.match(/\[/g) || []).length;
            let closeBrackets = (repaired.match(/\]/g) || []).length;
            
            for (let i = 0; i < (openBrackets - closeBrackets); i++) repaired += ']';
            for (let i = 0; i < (openBraces - closeBraces); i++) repaired += '}';
            
            try {
                return JSON.parse(repaired);
            } catch (err) {
                console.error("❌ Auto-Heal Failed. Skipping this tick.");
                return null;
            }
          }
      };
    </script>
    </body>
  </html>)rawliteral";

#define COMMON_JAVASCRIPT \
  R"rawliteral(
<script>
</script>
)rawliteral"

extern const char index_html[];
extern const char index_html_header[];
extern const char index_html_footer[];

#endif  // INDEX_HTML_H
