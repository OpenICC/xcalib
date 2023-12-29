# xcalib v0.11.0 {#xcalibde}
<a name="toc"></a>
[NAME](#name) [ÜBERSICHT](#synopsis) [BESCHREIBUNG](#description) [OPTIONEN](#options) [UMGEBUNGSVARIABLEN](#environmentvariables) [BEISPIELE](#examples) [SIEHE AUCH](#seealso) [AUTOR](#author) 

<strong>"xcalib"</strong> *1* <em>"14. Dezember 2023"</em> "User Commands"

<h2>NAME <a href="#toc" name="name">&uarr;</a></h2>

xcalib v0.11.0 - Monitor Kalibrationseinrichter

<h2>ÜBERSICHT <a href="#toc" name="synopsis">&uarr;</a></h2>

<strong>xcalib</strong> ICC_DATEI_NAME
<br />
<strong>xcalib</strong> <a href="#clear"><strong>-c</strong></a> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>]
<br />
<strong>xcalib</strong> <a href="#invert"><strong>-i</strong></a> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_DATEI_NAME | <strong>-a</strong>
<br />
<strong>xcalib</strong> <a href="#gamma"><strong>-g</strong>=<em>NUMMER</em></a> <strong>-b</strong>=<em>NUMMER</em> <strong>-k</strong>=<em>NUMMER</em> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_DATEI_NAME | <strong>-a</strong>
<br />
<strong>xcalib</strong> <a href="#red-gamma"><strong>-R</strong>=<em>NUMMER</em></a> <strong>-G</strong>=<em>NUMMER</em> <strong>-B</strong>=<em>NUMMER</em> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-S</strong>=<em>NUMMER</em>] [<strong>-T</strong>=<em>NUMMER</em>] [<strong>-H</strong>=<em>NUMMER</em>] [<strong>-I</strong>=<em>NUMMER</em>] [<strong>-C</strong>=<em>NUMMER</em>] [<strong>-D</strong>=<em>NUMMER</em>] [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_DATEI_NAME | <strong>-a</strong>
<br />
<strong>xcalib</strong> <a href="#printramps"><strong>-p</strong><em>[=FORMAT]</em></a> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>]
<br />
<strong>xcalib</strong> <a href="#help"><strong>-h</strong><em>[=synopsis|...]</em></a> <strong>-V</strong> <strong>--render</strong>=<em>TEXT</em> [<strong>-v</strong>]

<h2>BESCHREIBUNG <a href="#toc" name="description">&uarr;</a></h2>

Das Werkzeug lädt den vcgt-Tag eines ICC Farbprofils in den Bildserver mit der XRandR/XVidMode/GDI Erweiterung um die Grafikkarte passend zum Monitor farblich zu kalibrieren.

<h2>OPTIONEN <a href="#toc" name="options">&uarr;</a></h2>

<h3>Grundlegende Einstellungen</h3>


<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-d</strong>|<strong>--display</strong>=<em>TEXT</em></td> <td>host:dpy </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-s</strong>|<strong>--screen</strong>=<em>NUMMER</em></td> <td>Ausgabenummer </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-o</strong>|<strong>--output</strong>=<em>NUMMER</em></td> <td>Anschluss Nummer<br />Erscheint in der Reihenfolge wie im xrandr Werkzeug. </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-a</strong>|<strong>--alter</strong></td> <td>Andere Tabelle<br />Arbeitet alternativ zu einem ICC Profil nur mit den Parametern.</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-n</strong>|<strong>--noaction</strong></td> <td>Belasse die Gamma Tabelle<br />Arbeitet am besten zusammen mit der -v Option.</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-p</strong>|<strong>--printramps</strong><em>[=FORMAT]</em></td> <td>Zeige Werte auf der Standardausgabe stdout
  <table>
   <tr><td style='padding-left:0.5em'><strong>-p</strong> TEXT</td><td># TEXT</td></tr>
   <tr><td style='padding-left:0.5em'><strong>-p</strong> SVG</td><td># SVG</td></tr>
  </table>
  </td>
 </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-l</strong>|<strong>--loss</strong></td> <td>Zeige Verluste durch die Anwendung der Kurven auf stdout.</td> </tr>
</table>

<h3>Zuweisen</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> ICC_DATEI_NAME

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><em>ICC_DATEI_NAME</em></td> <td>Dateiname eines ICC Farbprofiles </tr>
</table>

<h3 id="clear">Zurücksetzen</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-c</strong> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>]

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-c</strong>|<strong>--clear</strong></td> <td>Setze Gamma Tabelle zurück<br />Setze die Video Card Gamma Tablle (VCGT) auf lineare Werte.</td> </tr>
</table>

<h3 id="invert">Umkehren</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-i</strong> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_DATEI_NAME | <strong>-a</strong>

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-i</strong>|<strong>--invert</strong></td> <td>Kehre die Gamma Tabelle um</td> </tr>
</table>

<h3 id="gamma">Allgemeine Farbanpassung</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-g</strong>=<em>NUMMER</em> <strong>-b</strong>=<em>NUMMER</em> <strong>-k</strong>=<em>NUMMER</em> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_DATEI_NAME | <strong>-a</strong>

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-g</strong>|<strong>--gamma</strong>=<em>NUMMER</em></td> <td>Setze Gamma Kurve: Global gamma correction value (use 2.2 for WinXP Color Control-like behaviour) (NUMMER:1 [≥0,1 ≤5 Δ0,1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-b</strong>|<strong>--brightness</strong>=<em>NUMMER</em></td> <td>Setze Helligkeit in Prozent (NUMMER:0 [≥0 ≤99 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-k</strong>|<strong>--contrast</strong>=<em>NUMMER</em></td> <td>Setze Kontrast in Prozent: Setze den maxmalen Werten im Verhältnis zur Helligkeit. (NUMMER:100 [≥1 ≤100 Δ1])</td> </tr>
</table>

<h3 id="red-gamma">Farbanpassung pro Farbkanal</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-R</strong>=<em>NUMMER</em> <strong>-G</strong>=<em>NUMMER</em> <strong>-B</strong>=<em>NUMMER</em> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-S</strong>=<em>NUMMER</em>] [<strong>-T</strong>=<em>NUMMER</em>] [<strong>-H</strong>=<em>NUMMER</em>] [<strong>-I</strong>=<em>NUMMER</em>] [<strong>-C</strong>=<em>NUMMER</em>] [<strong>-D</strong>=<em>NUMMER</em>] [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>] [<strong>-n</strong>] [<strong>-p</strong><em>[=FORMAT]</em>] [<strong>-l</strong>] ICC_DATEI_NAME | <strong>-a</strong>

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-R</strong>|<strong>--red-gamma</strong>=<em>NUMMER</em></td> <td>Setze Rote Gamma Kurve (NUMMER:1 [≥0,1 ≤5 Δ0,1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-S</strong>|<strong>--red-brightness</strong>=<em>NUMMER</em></td> <td>Setze Rote Helligkeit in Prozent (NUMMER:0 [≥0 ≤99 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-T</strong>|<strong>--red-contrast</strong>=<em>NUMMER</em></td> <td>Setze Roten Kontrast in Prozent: Setze den maxmalen Werten im Verhältnis zur Helligkeit. (NUMMER:100 [≥1 ≤100 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-G</strong>|<strong>--green-gamma</strong>=<em>NUMMER</em></td> <td>Setze Grüne Gamma Kurve (NUMMER:1 [≥0,1 ≤5 Δ0,1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-H</strong>|<strong>--green-brightness</strong>=<em>NUMMER</em></td> <td>Setze Grüne Helligkeit in Prozent (NUMMER:0 [≥0 ≤99 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-I</strong>|<strong>--green-contrast</strong>=<em>NUMMER</em></td> <td>Setze Grünen Kontrast in Prozent: Setze den maxmalen Werten im Verhältnis zur Helligkeit. (NUMMER:100 [≥1 ≤100 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-B</strong>|<strong>--blue-gamma</strong>=<em>NUMMER</em></td> <td>Setze Blaue Gamma Kurve (NUMMER:1 [≥0,1 ≤5 Δ0,1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-C</strong>|<strong>--blue-brightness</strong>=<em>NUMMER</em></td> <td>Setze Blaue Helligkeit in Prozent (NUMMER:0 [≥0 ≤99 Δ1])</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-D</strong>|<strong>--blue-contrast</strong>=<em>NUMMER</em></td> <td>Setze Blauen Kontrast in Prozent: Setze den maxmalen Werten im Verhältnis zur Helligkeit. (NUMMER:100 [≥1 ≤100 Δ1])</td> </tr>
</table>

<h3 id="printramps">Show</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-p</strong><em>[=FORMAT]</em> <strong>-d</strong>=<em>TEXT</em> <strong>-s</strong>=<em>NUMMER</em> [<strong>-o</strong>=<em>NUMMER</em>] [<strong>-v</strong>]

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-p</strong>|<strong>--printramps</strong><em>[=FORMAT]</em></td> <td>Zeige Werte auf der Standardausgabe stdout
  <table>
   <tr><td style='padding-left:0.5em'><strong>-p</strong> TEXT</td><td># TEXT</td></tr>
   <tr><td style='padding-left:0.5em'><strong>-p</strong> SVG</td><td># SVG</td></tr>
  </table>
  </td>
 </tr>
</table>

<h3 id="help">Allgemeine Optionen</h3>

&nbsp;&nbsp; <a href="#synopsis"><strong>xcalib</strong></a> <strong>-h</strong><em>[=synopsis|...]</em> <strong>-V</strong> <strong>--render</strong>=<em>TEXT</em> [<strong>-v</strong>]

<table style='width:100%'>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-h</strong>|<strong>--help</strong><em>[=synopsis|...]</em></td> <td>Zeige Hilfetext an<br />Zeige Benutzungsinformationen und Hinweise für das Werkzeug.
  <table>
   <tr><td style='padding-left:0.5em'><strong>-h</strong> -</td><td># Vollständige Hilfe : Zeige Hilfe für alle Gruppen</td></tr>
   <tr><td style='padding-left:0.5em'><strong>-h</strong> synopsis</td><td># Übersicht : Liste Gruppen - Zeige alle Gruppen mit Syntax</td></tr>
  </table>
  </td>
 </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>--render</strong>=<em>TEXT</em></td> <td>  </td>
 </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-V</strong>|<strong>--version</strong></td> <td>Version</td> </tr>
 <tr><td style='padding-left:1em;padding-right:1em;vertical-align:top;width:25%'><strong>-v</strong>|<strong>--verbose</strong></td> <td>Plaudernd</td> </tr>
</table>


<h2>UMGEBUNGSVARIABLEN <a href="#toc" name="environmentvariables">&uarr;</a></h2>

#### DISPLAY
&nbsp;&nbsp;Diese Variable enthält den Ausgabenamen unter X11 Systemen wie für die -d und -s Optionen benutzt.

<h2>BEISPIELE <a href="#toc" name="examples">&uarr;</a></h2>

#### Weise die VCGT Kurven aus einem ICC Profil einem Bildschirm zu
&nbsp;&nbsp;xcalib ‐d :0 ‐s 0 ‐v profil_mit_vcgt_tag.icc
#### Setze die Geräte-LUT einer Anzeige für eine Kalibration zurück.
&nbsp;&nbsp;xcalib ‐d :0 ‐s 0 ‐c

<h2>SIEHE AUCH <a href="#toc" name="seealso">&uarr;</a></h2>

&nbsp;&nbsp;[oyjl-args](oyjlargs.html)<a href="oyjlargs.md">(1)</a>


<h2>AUTOR <a href="#toc" name="author">&uarr;</a></h2>

Stefan Dohla <stefan AT doehla DOT de> http://www.etg.e‐technik.uni‐erlangen.de/web/doe/xcalib/

