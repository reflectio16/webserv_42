# Webserv — Roadmap François

## Phase A — Finir le Config Parser

### ÉTAPE 10 — Définir `LocationBlock`
Déterminer les données à stocker pour une route : `path`, `methods`, `root`, `index`, `autoindex`, `upload_dir`, redirection, CGI.

### ÉTAPE 11 — Ajouter les `locations` à `ServerBlock`
Ajouter quelque chose comme `std::vector<LocationBlock>` et réfléchir à l’héritage éventuel du `root` serveur.

### ÉTAPE 12 — Parser `location /path { ... }`
Reconnaître les sous-blocs imbriqués correctement. Ici, la notion de profondeur / blocs devient vraiment importante.

### ÉTAPE 13 — Parser `methods`
Stocker les méthodes autorisées : GET, POST, DELETE.

### ÉTAPE 14 — Parser `root` et `index` dans une location
Différencier les directives au niveau `server` et au niveau `location`.

### ÉTAPE 15 — Parser `autoindex`
Reconnaître `on/off`, stocker un booléen et valider les valeurs.

### ÉTAPE 16 — Parser `client_max_body_size`
Stocker la limite de body au niveau serveur, avec conversion et validation.

### ÉTAPE 17 — Parser `error_page`
Associer un code HTTP à un chemin, probablement avec une `std::map<int, std::string>`.

### ÉTAPE 18 — Parser `upload_dir`
Stocker la destination des uploads.

### ÉTAPE 19 — Parser les redirections
Code + destination, par exemple `301 /new`.

### ÉTAPE 20 — Parser la configuration CGI
Extension concernée + exécutable/interpréteur selon le format que vous choisissez.

### ÉTAPE 21 — Validation complète de la config
Doublons, directives interdites selon le niveau, blocs mal fermés, valeurs invalides, locations invalides, etc.

### ÉTAPE 22 — Ajouter les getters utiles
Pas seulement `getEndpoints()`, mais aussi des moyens propres de récupérer un `ServerBlock` et surtout la `LocationBlock` correspondant à une URI.

### ÉTAPE 23 — Tester fortement le parser de config
Configs valides, invalides, espaces étranges, accolades, directives manquantes, valeurs limites.

---

## Phase B — HTTP Request

### ÉTAPE 24 — Définir `HttpRequest`
Réfléchir à ce qu’une requête doit contenir : méthode, URI, version, headers, body, éventuellement query string.

### ÉTAPE 25 — Définir les états du `RequestParser`
Par exemple : headers incomplets, body Content-Length, body chunked, complete, error.

### ÉTAPE 26 — Parser la request line
`GET /index.html HTTP/1.1`.

### ÉTAPE 27 — Parser les headers
Transformer les lignes `Name: value` en structure exploitable.

### ÉTAPE 28 — Détecter la fin des headers
`\r\n\r\n`, mais sans confondre ça avec la fin de toute la requête.

### ÉTAPE 29 — Gérer `Content-Length`
Savoir combien d’octets de body attendre.

### ÉTAPE 30 — Supporter les `recv()` fragmentés
Le parser doit pouvoir recevoir `"GET /ind"` puis `"ex.html..."` sans casser.

### ÉTAPE 31 — Parser le body normal
Accumulation progressive jusqu’à la taille attendue.

### ÉTAPE 32 — Implémenter `Transfer-Encoding: chunked`
Taille hexadécimale, chunks partiels, chunk 0, reconstruction du vrai body.

### ÉTAPE 33 — Appliquer `client_max_body_size`
Réponse potentielle `413`, y compris pendant le chunked.

### ÉTAPE 34 — Parser URI et query string
Par exemple `/hello.py?user=Bob` → chemin + `user=Bob`.

### ÉTAPE 35 — Validation HTTP et erreurs de parsing
400, méthode inconnue, version invalide, headers mal formés, framing incohérent, etc.

### ÉTAPE 36 — `reset()` du parser
Pour permettre à Merve de réutiliser la connexion en keep-alive.

---

## Phase C — Routage et réponses HTTP

### ÉTAPE 37 — Définir une structure `HttpResponse` ou le contrat du `ResponseBuilder`
Status, headers, body.

### ÉTAPE 38 — Construire une réponse HTTP minimale
Par exemple `200 OK` + `Content-Length`.

### ÉTAPE 39 — Implémenter le matching des locations
Trouver la route la plus spécifique correspondant à l’URI.

### ÉTAPE 40 — Résoudre URI → chemin filesystem
Exemple `/images/cat.jpg` + root de location.

### ÉTAPE 41 — GET fichier statique
Lire un fichier et construire une réponse.

### ÉTAPE 42 — MIME types / `Content-Type`
HTML, CSS, JS, PNG, JPEG, etc.

### ÉTAPE 43 — Pages d’erreur
404, 403, 405, 413, 500… personnalisées ou défaut.

### ÉTAPE 44 — `index` pour les répertoires
Chercher `index.html`, etc.

### ÉTAPE 45 — `autoindex`
Générer une page HTML listant le contenu d’un répertoire.

### ÉTAPE 46 — Redirections HTTP
Construire une vraie réponse 301/302 avec `Location`.

### ÉTAPE 47 — Vérifier les méthodes autorisées
Répondre `405 Method Not Allowed` selon la location.

### ÉTAPE 48 — DELETE
Résoudre la cible, vérifier les règles, supprimer la ressource, produire le bon statut.

### ÉTAPE 49 — POST / upload
Enregistrer le body dans le bon dossier selon la config.

### ÉTAPE 50 — Multipart si nécessaire
À décider selon vos tests et le comportement d’upload attendu.

---

## Phase D — CGI + intégration + robustesse

### ÉTAPE 51 — Construire les variables d’environnement CGI
`REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`, etc.

### ÉTAPE 52 — Préparer le body à transmettre au CGI
Notamment body déjà un-chunked.

### ÉTAPE 53 — Intégration avec `CgiProcess` de Merve
Toi : données/protocole HTTP. Merve : pipes/fork/exec.

### ÉTAPE 54 — Parser la sortie CGI
Headers CGI + body.

### ÉTAPE 55 — Transformer la sortie CGI en réponse HTTP
Gérer notamment l’absence de `Content-Length`.

### ÉTAPE 56 — Gestion des erreurs CGI
Script absent, exec échoué, timeout, sortie invalide.

### ÉTAPE 57 — Keep-alive côté HTTP
Décider quand conserver ou fermer la connexion.

### ÉTAPE 58 — Intégration complète `RequestParser` ↔ `Connection` ↔ `ResponseBuilder`
C’est là que ta partie et celle de Merve se rejoignent vraiment.

### ÉTAPE 59 — Tests avec `curl`, navigateur et `telnet`
GET, POST, DELETE, headers, erreurs, connexions lentes.

### ÉTAPE 60 — Tests automatisés
Petit script Python qui envoie des requêtes valides et volontairement cassées.

### ÉTAPE 61 — Stress tests / résilience
Clients simultanés, déconnexions, gros bodies, CGI lent, requêtes fragmentées.

### ÉTAPE 62 — Nettoyage final
Leaks, fd fermés, erreurs, conformité C++98, Makefile, fichiers de démonstration pour l’évaluation.

---

## Vue synthétique

```text
CONFIG
10 → 23

HTTP REQUEST
24 → 36

ROUTING / RESPONSE
37 → 50

CGI / INTEGRATION / TESTS
51 → 62
```
