var qo=Object.defineProperty;var un=e=>{throw TypeError(e)};var Ko=(e,t,r)=>t in e?qo(e,t,{enumerable:!0,configurable:!0,writable:!0,value:r}):e[t]=r;var vt=(e,t,r)=>Ko(e,typeof t!="symbol"?t+"":t,r),Sa=(e,t,r)=>t.has(e)||un("Cannot "+r);var p=(e,t,r)=>(Sa(e,t,"read from private field"),r?r.call(e):t.get(e)),K=(e,t,r)=>t.has(e)?un("Cannot add the same private member more than once"):t instanceof WeakSet?t.add(e):t.set(e,r),Q=(e,t,r,a)=>(Sa(e,t,"write to private field"),a?a.call(e,r):t.set(e,r),r),le=(e,t,r)=>(Sa(e,t,"access private method"),r);(function(){const t=document.createElement("link").relList;if(t&&t.supports&&t.supports("modulepreload"))return;for(const n of document.querySelectorAll('link[rel="modulepreload"]'))a(n);new MutationObserver(n=>{for(const o of n)if(o.type==="childList")for(const i of o.addedNodes)i.tagName==="LINK"&&i.rel==="modulepreload"&&a(i)}).observe(document,{childList:!0,subtree:!0});function r(n){const o={};return n.integrity&&(o.integrity=n.integrity),n.referrerPolicy&&(o.referrerPolicy=n.referrerPolicy),n.crossOrigin==="use-credentials"?o.credentials="include":n.crossOrigin==="anonymous"?o.credentials="omit":o.credentials="same-origin",o}function a(n){if(n.ep)return;n.ep=!0;const o=r(n);fetch(n.href,o)}})();const Go="5";var Cn;typeof window<"u"&&((Cn=window.__svelte??(window.__svelte={})).v??(Cn.v=new Set)).add(Go);const Yo=1,Jo=2,Dn=4,Zo=8,Xo=16,Qo=1,es=2,zn=4,ts=8,rs=16,as=1,ns=2,we=Symbol(),jn="http://www.w3.org/1999/xhtml",os="http://www.w3.org/2000/svg",ss="@attach",is=!1;var Ya=Array.isArray,ls=Array.prototype.indexOf,kr=Array.prototype.includes,ma=Array.from,cs=Object.defineProperty,qt=Object.getOwnPropertyDescriptor,Fn=Object.getOwnPropertyDescriptors,ds=Object.prototype,us=Array.prototype,Ja=Object.getPrototypeOf,fn=Object.isExtensible;function Cr(e){return typeof e=="function"}const cr=()=>{};function fs(e){return e()}function Ia(e){for(var t=0;t<e.length;t++)e[t]()}function Bn(){var e,t,r=new Promise((a,n)=>{e=a,t=n});return{promise:r,resolve:e,reject:t}}function Hn(e,t){if(Array.isArray(e))return e;if(!(Symbol.iterator in e))return Array.from(e);const r=[];for(const a of e)if(r.push(a),r.length===t)break;return r}const Le=2,Sr=4,Xr=8,Za=1<<24,Et=16,xt=32,Gt=64,Oa=128,it=512,$e=1024,Ie=2048,At=4096,Fe=8192,Qe=16384,_r=32768,vn=1<<25,Yt=65536,Ca=1<<17,vs=1<<18,Ir=1<<19,Vn=1<<20,St=1<<25,vr=65536,La=1<<21,Vr=1<<22,Kt=1<<23,Ct=Symbol("$state"),Un=Symbol("legacy props"),ps=Symbol(""),Tt=new class extends Error{constructor(){super(...arguments);vt(this,"name","StaleReactionError");vt(this,"message","The reaction that called `getAbortSignal()` was re-run or destroyed")}};var Ln;const Xa=!!((Ln=globalThis.document)!=null&&Ln.contentType)&&globalThis.document.contentType.includes("xml");function hs(){throw new Error("https://svelte.dev/e/async_derived_orphan")}function _s(e,t,r){throw new Error("https://svelte.dev/e/each_key_duplicate")}function xs(e){throw new Error("https://svelte.dev/e/effect_in_teardown")}function bs(){throw new Error("https://svelte.dev/e/effect_in_unowned_derived")}function ms(e){throw new Error("https://svelte.dev/e/effect_orphan")}function gs(){throw new Error("https://svelte.dev/e/effect_update_depth_exceeded")}function ys(e){throw new Error("https://svelte.dev/e/props_invalid_value")}function ws(){throw new Error("https://svelte.dev/e/state_descriptors_fixed")}function $s(){throw new Error("https://svelte.dev/e/state_prototype_fixed")}function ks(){throw new Error("https://svelte.dev/e/state_unsafe_mutation")}function Ss(){throw new Error("https://svelte.dev/e/svelte_boundary_reset_onerror")}function Es(){console.warn("https://svelte.dev/e/derived_inert")}function As(){console.warn("https://svelte.dev/e/select_multiple_invalid_value")}function Ns(){console.warn("https://svelte.dev/e/svelte_boundary_reset_noop")}function Wn(e){return e===this.v}function Ts(e,t){return e!=e?t==t:e!==t||e!==null&&typeof e=="object"||typeof e=="function"}function qn(e){return!Ts(e,this.v)}let Qr=!1,Ms=!1;function Ps(){Qr=!0}let Ae=null;function Er(e){Ae=e}function He(e,t=!1,r){Ae={p:Ae,i:!1,c:null,e:null,s:e,x:null,r:J,l:Qr&&!t?{s:null,u:null,$:[]}:null}}function Ve(e){var t=Ae,r=t.e;if(r!==null){t.e=null;for(var a of r)po(a)}return t.i=!0,Ae=t.p,{}}function ea(){return!Qr||Ae!==null&&Ae.l===null}let tr=[];function Kn(){var e=tr;tr=[],Ia(e)}function Lt(e){if(tr.length===0&&!Fr){var t=tr;queueMicrotask(()=>{t===tr&&Kn()})}tr.push(e)}function Is(){for(;tr.length>0;)Kn()}function Gn(e){var t=J;if(t===null)return ee.f|=Kt,e;if((t.f&_r)===0&&(t.f&Sr)===0)throw e;Wt(e,t)}function Wt(e,t){for(;t!==null;){if((t.f&Oa)!==0){if((t.f&_r)===0)throw e;try{t.b.error(e);return}catch(r){e=r}}t=t.parent}throw e}const Os=-7169;function _e(e,t){e.f=e.f&Os|t}function Qa(e){(e.f&it)!==0||e.deps===null?_e(e,$e):_e(e,At)}function Yn(e){if(e!==null)for(const t of e)(t.f&Le)===0||(t.f&vr)===0||(t.f^=vr,Yn(t.deps))}function Jn(e,t,r){(e.f&Ie)!==0?t.add(e):(e.f&At)!==0&&r.add(e),Yn(e.deps),_e(e,$e)}let oa=!1;function Cs(e){var t=oa;try{return oa=!1,[e(),oa]}finally{oa=t}}const Qt=new Set;let V=null,Pe=null,Ra=null,Fr=!1,Ea=!1,br=null,ia=null;var pn=0;let Ls=1;var mr,gr,nr,Mt,wt,qr,Je,Kr,Vt,Pt,$t,yr,wr,or,be,la,Zn,ca,Da,da,Rs;const _a=class _a{constructor(){K(this,be);vt(this,"id",Ls++);vt(this,"current",new Map);vt(this,"previous",new Map);K(this,mr,new Set);K(this,gr,new Set);K(this,nr,new Set);K(this,Mt,new Map);K(this,wt,new Map);K(this,qr,null);K(this,Je,[]);K(this,Kr,[]);K(this,Vt,new Set);K(this,Pt,new Set);K(this,$t,new Map);K(this,yr,new Set);vt(this,"is_fork",!1);K(this,wr,!1);K(this,or,new Set)}skip_effect(t){p(this,$t).has(t)||p(this,$t).set(t,{d:[],m:[]}),p(this,yr).delete(t)}unskip_effect(t,r=a=>this.schedule(a)){var a=p(this,$t).get(t);if(a){p(this,$t).delete(t);for(var n of a.d)_e(n,Ie),r(n);for(n of a.m)_e(n,At),r(n)}p(this,yr).add(t)}capture(t,r,a=!1){t.v!==we&&!this.previous.has(t)&&this.previous.set(t,t.v),(t.f&Kt)===0&&(this.current.set(t,[r,a]),Pe==null||Pe.set(t,r)),this.is_fork||(t.v=r)}activate(){V=this}deactivate(){V=null,Pe=null}flush(){try{Ea=!0,V=this,le(this,be,ca).call(this)}finally{pn=0,Ra=null,br=null,ia=null,Ea=!1,V=null,Pe=null,dr.clear()}}discard(){for(const t of p(this,gr))t(this);p(this,gr).clear(),p(this,nr).clear(),Qt.delete(this)}register_created_effect(t){p(this,Kr).push(t)}increment(t,r){let a=p(this,Mt).get(r)??0;if(p(this,Mt).set(r,a+1),t){let n=p(this,wt).get(r)??0;p(this,wt).set(r,n+1)}}decrement(t,r,a){let n=p(this,Mt).get(r)??0;if(n===1?p(this,Mt).delete(r):p(this,Mt).set(r,n-1),t){let o=p(this,wt).get(r)??0;o===1?p(this,wt).delete(r):p(this,wt).set(r,o-1)}p(this,wr)||a||(Q(this,wr,!0),Lt(()=>{Q(this,wr,!1),this.flush()}))}transfer_effects(t,r){for(const a of t)p(this,Vt).add(a);for(const a of r)p(this,Pt).add(a);t.clear(),r.clear()}oncommit(t){p(this,mr).add(t)}ondiscard(t){p(this,gr).add(t)}on_fork_commit(t){p(this,nr).add(t)}run_fork_commit_callbacks(){for(const t of p(this,nr))t(this);p(this,nr).clear()}settled(){return(p(this,qr)??Q(this,qr,Bn())).promise}static ensure(){if(V===null){const t=V=new _a;Ea||(Qt.add(V),Fr||Lt(()=>{V===t&&t.flush()}))}return V}apply(){{Pe=null;return}}schedule(t){var n;if(Ra=t,(n=t.b)!=null&&n.is_pending&&(t.f&(Sr|Xr|Za))!==0&&(t.f&_r)===0){t.b.defer_effect(t);return}for(var r=t;r.parent!==null;){r=r.parent;var a=r.f;if(br!==null&&r===J&&(ee===null||(ee.f&Le)===0))return;if((a&(Gt|xt))!==0){if((a&$e)===0)return;r.f^=$e}}p(this,Je).push(r)}};mr=new WeakMap,gr=new WeakMap,nr=new WeakMap,Mt=new WeakMap,wt=new WeakMap,qr=new WeakMap,Je=new WeakMap,Kr=new WeakMap,Vt=new WeakMap,Pt=new WeakMap,$t=new WeakMap,yr=new WeakMap,wr=new WeakMap,or=new WeakMap,be=new WeakSet,la=function(){return this.is_fork||p(this,wt).size>0},Zn=function(){for(const a of p(this,or))for(const n of p(a,wt).keys()){for(var t=!1,r=n;r.parent!==null;){if(p(this,$t).has(r)){t=!0;break}r=r.parent}if(!t)return!0}return!1},ca=function(){var l,c;if(pn++>1e3&&(Qt.delete(this),zs()),!le(this,be,la).call(this)){for(const d of p(this,Vt))p(this,Pt).delete(d),_e(d,Ie),this.schedule(d);for(const d of p(this,Pt))_e(d,At),this.schedule(d)}const t=p(this,Je);Q(this,Je,[]),this.apply();var r=br=[],a=[],n=ia=[];for(const d of t)try{le(this,be,Da).call(this,d,r,a)}catch(v){throw eo(d),v}if(V=null,n.length>0){var o=_a.ensure();for(const d of n)o.schedule(d)}if(br=null,ia=null,le(this,be,la).call(this)||le(this,be,Zn).call(this)){le(this,be,da).call(this,a),le(this,be,da).call(this,r);for(const[d,v]of p(this,$t))Qn(d,v)}else{p(this,Mt).size===0&&Qt.delete(this),p(this,Vt).clear(),p(this,Pt).clear();for(const d of p(this,mr))d(this);p(this,mr).clear(),hn(a),hn(r),(l=p(this,qr))==null||l.resolve()}var i=V;if(p(this,Je).length>0){const d=i??(i=this);p(d,Je).push(...p(this,Je).filter(v=>!p(d,Je).includes(v)))}i!==null&&(Qt.add(i),le(c=i,be,ca).call(c))},Da=function(t,r,a){t.f^=$e;for(var n=t.first;n!==null;){var o=n.f,i=(o&(xt|Gt))!==0,l=i&&(o&$e)!==0,c=l||(o&Fe)!==0||p(this,$t).has(n);if(!c&&n.fn!==null){i?n.f^=$e:(o&Sr)!==0?r.push(n):na(n)&&((o&Et)!==0&&p(this,Pt).add(n),Tr(n));var d=n.first;if(d!==null){n=d;continue}}for(;n!==null;){var v=n.next;if(v!==null){n=v;break}n=n.parent}}},da=function(t){for(var r=0;r<t.length;r+=1)Jn(t[r],p(this,Vt),p(this,Pt))},Rs=function(){var v,m,_;for(const g of Qt){var t=g.id<this.id,r=[];for(const[h,[$,x]]of this.current){if(g.current.has(h)){var a=g.current.get(h)[0];if(t&&$!==a)g.current.set(h,[$,x]);else continue}r.push(h)}var n=[...g.current.keys()].filter(h=>!this.current.has(h));if(n.length===0)t&&g.discard();else if(r.length>0){if(t)for(const h of p(this,yr))g.unskip_effect(h,$=>{var x;($.f&(Et|Vr))!==0?g.schedule($):le(x=g,be,da).call(x,[$])});g.activate();var o=new Set,i=new Map;for(var l of r)Xn(l,n,o,i);i=new Map;var c=[...g.current.keys()].filter(h=>this.current.has(h)?this.current.get(h)[0]!==h:!0);for(const h of p(this,Kr))(h.f&(Qe|Fe|Ca))===0&&en(h,c,i)&&((h.f&(Vr|Et))!==0?(_e(h,Ie),g.schedule(h)):p(g,Vt).add(h));if(p(g,Je).length>0){g.apply();for(var d of p(g,Je))le(v=g,be,Da).call(v,d,[],[]);Q(g,Je,[])}g.deactivate()}}for(const g of Qt)p(g,or).has(this)&&(p(g,or).delete(this),p(g,or).size===0&&!le(m=g,be,la).call(m)&&(g.activate(),le(_=g,be,ca).call(_)))};let pr=_a;function Ds(e){var t=Fr;Fr=!0;try{for(var r;;){if(Is(),V===null)return r;V.flush()}}finally{Fr=t}}function zs(){try{gs()}catch(e){Wt(e,Ra)}}let pt=null;function hn(e){var t=e.length;if(t!==0){for(var r=0;r<t;){var a=e[r++];if((a.f&(Qe|Fe))===0&&na(a)&&(pt=new Set,Tr(a),a.deps===null&&a.first===null&&a.nodes===null&&a.teardown===null&&a.ac===null&&mo(a),(pt==null?void 0:pt.size)>0)){dr.clear();for(const n of pt){if((n.f&(Qe|Fe))!==0)continue;const o=[n];let i=n.parent;for(;i!==null;)pt.has(i)&&(pt.delete(i),o.push(i)),i=i.parent;for(let l=o.length-1;l>=0;l--){const c=o[l];(c.f&(Qe|Fe))===0&&Tr(c)}}pt.clear()}}pt=null}}function Xn(e,t,r,a){if(!r.has(e)&&(r.add(e),e.reactions!==null))for(const n of e.reactions){const o=n.f;(o&Le)!==0?Xn(n,t,r,a):(o&(Vr|Et))!==0&&(o&Ie)===0&&en(n,t,a)&&(_e(n,Ie),tn(n))}}function en(e,t,r){const a=r.get(e);if(a!==void 0)return a;if(e.deps!==null)for(const n of e.deps){if(kr.call(t,n))return!0;if((n.f&Le)!==0&&en(n,t,r))return r.set(n,!0),!0}return r.set(e,!1),!1}function tn(e){V.schedule(e)}function Qn(e,t){if(!((e.f&xt)!==0&&(e.f&$e)!==0)){(e.f&Ie)!==0?t.d.push(e):(e.f&At)!==0&&t.m.push(e),_e(e,$e);for(var r=e.first;r!==null;)Qn(r,t),r=r.next}}function eo(e){_e(e,$e);for(var t=e.first;t!==null;)eo(t),t=t.next}function js(e){let t=0,r=Jt(0),a;return()=>{nn()&&(s(r),_o(()=>(t===0&&(a=Mr(()=>e(()=>Br(r)))),t+=1,()=>{Lt(()=>{t-=1,t===0&&(a==null||a(),a=void 0,Br(r))})})))}}var Fs=Yt|Ir;function Bs(e,t,r,a){new Hs(e,t,r,a)}var nt,Ga,ot,sr,Ue,st,je,Ze,It,ir,Ut,$r,Gr,Yr,Ot,xa,pe,Vs,Us,Ws,za,ua,fa,ja,Fa;class Hs{constructor(t,r,a,n){K(this,pe);vt(this,"parent");vt(this,"is_pending",!1);vt(this,"transform_error");K(this,nt);K(this,Ga,null);K(this,ot);K(this,sr);K(this,Ue);K(this,st,null);K(this,je,null);K(this,Ze,null);K(this,It,null);K(this,ir,0);K(this,Ut,0);K(this,$r,!1);K(this,Gr,new Set);K(this,Yr,new Set);K(this,Ot,null);K(this,xa,js(()=>(Q(this,Ot,Jt(p(this,ir))),()=>{Q(this,Ot,null)})));var o;Q(this,nt,t),Q(this,ot,r),Q(this,sr,i=>{var l=J;l.b=this,l.f|=Oa,a(i)}),this.parent=J.b,this.transform_error=n??((o=this.parent)==null?void 0:o.transform_error)??(i=>i),Q(this,Ue,aa(()=>{le(this,pe,za).call(this)},Fs))}defer_effect(t){Jn(t,p(this,Gr),p(this,Yr))}is_rendered(){return!this.is_pending&&(!this.parent||this.parent.is_rendered())}has_pending_snippet(){return!!p(this,ot).pending}update_pending_count(t,r){le(this,pe,ja).call(this,t,r),Q(this,ir,p(this,ir)+t),!(!p(this,Ot)||p(this,$r))&&(Q(this,$r,!0),Lt(()=>{Q(this,$r,!1),p(this,Ot)&&Ar(p(this,Ot),p(this,ir))}))}get_effect_pending(){return p(this,xa).call(this),s(p(this,Ot))}error(t){if(!p(this,ot).onerror&&!p(this,ot).failed)throw t;V!=null&&V.is_fork?(p(this,st)&&V.skip_effect(p(this,st)),p(this,je)&&V.skip_effect(p(this,je)),p(this,Ze)&&V.skip_effect(p(this,Ze)),V.on_fork_commit(()=>{le(this,pe,Fa).call(this,t)})):le(this,pe,Fa).call(this,t)}}nt=new WeakMap,Ga=new WeakMap,ot=new WeakMap,sr=new WeakMap,Ue=new WeakMap,st=new WeakMap,je=new WeakMap,Ze=new WeakMap,It=new WeakMap,ir=new WeakMap,Ut=new WeakMap,$r=new WeakMap,Gr=new WeakMap,Yr=new WeakMap,Ot=new WeakMap,xa=new WeakMap,pe=new WeakSet,Vs=function(){try{Q(this,st,qe(()=>p(this,sr).call(this,p(this,nt))))}catch(t){this.error(t)}},Us=function(t){const r=p(this,ot).failed;r&&Q(this,Ze,qe(()=>{r(p(this,nt),()=>t,()=>()=>{})}))},Ws=function(){const t=p(this,ot).pending;t&&(this.is_pending=!0,Q(this,je,qe(()=>t(p(this,nt)))),Lt(()=>{var r=Q(this,It,document.createDocumentFragment()),a=Rt();r.append(a),Q(this,st,le(this,pe,fa).call(this,()=>qe(()=>p(this,sr).call(this,a)))),p(this,Ut)===0&&(p(this,nt).before(r),Q(this,It,null),ur(p(this,je),()=>{Q(this,je,null)}),le(this,pe,ua).call(this,V))}))},za=function(){try{if(this.is_pending=this.has_pending_snippet(),Q(this,Ut,0),Q(this,ir,0),Q(this,st,qe(()=>{p(this,sr).call(this,p(this,nt))})),p(this,Ut)>0){var t=Q(this,It,document.createDocumentFragment());ln(p(this,st),t);const r=p(this,ot).pending;Q(this,je,qe(()=>r(p(this,nt))))}else le(this,pe,ua).call(this,V)}catch(r){this.error(r)}},ua=function(t){this.is_pending=!1,t.transfer_effects(p(this,Gr),p(this,Yr))},fa=function(t){var r=J,a=ee,n=Ae;dt(p(this,Ue)),ct(p(this,Ue)),Er(p(this,Ue).ctx);try{return pr.ensure(),t()}catch(o){return Gn(o),null}finally{dt(r),ct(a),Er(n)}},ja=function(t,r){var a;if(!this.has_pending_snippet()){this.parent&&le(a=this.parent,pe,ja).call(a,t,r);return}Q(this,Ut,p(this,Ut)+t),p(this,Ut)===0&&(le(this,pe,ua).call(this,r),p(this,je)&&ur(p(this,je),()=>{Q(this,je,null)}),p(this,It)&&(p(this,nt).before(p(this,It)),Q(this,It,null)))},Fa=function(t){p(this,st)&&(Oe(p(this,st)),Q(this,st,null)),p(this,je)&&(Oe(p(this,je)),Q(this,je,null)),p(this,Ze)&&(Oe(p(this,Ze)),Q(this,Ze,null));var r=p(this,ot).onerror;let a=p(this,ot).failed;var n=!1,o=!1;const i=()=>{if(n){Ns();return}n=!0,o&&Ss(),p(this,Ze)!==null&&ur(p(this,Ze),()=>{Q(this,Ze,null)}),le(this,pe,fa).call(this,()=>{le(this,pe,za).call(this)})},l=c=>{try{o=!0,r==null||r(c,i),o=!1}catch(d){Wt(d,p(this,Ue)&&p(this,Ue).parent)}a&&Q(this,Ze,le(this,pe,fa).call(this,()=>{try{return qe(()=>{var d=J;d.b=this,d.f|=Oa,a(p(this,nt),()=>c,()=>i)})}catch(d){return Wt(d,p(this,Ue).parent),null}}))};Lt(()=>{var c;try{c=this.transform_error(t)}catch(d){Wt(d,p(this,Ue)&&p(this,Ue).parent);return}c!==null&&typeof c=="object"&&typeof c.then=="function"?c.then(l,d=>Wt(d,p(this,Ue)&&p(this,Ue).parent)):l(c)})};function to(e,t,r,a){const n=ea()?ta:rn;var o=e.filter(_=>!_.settled);if(r.length===0&&o.length===0){a(t.map(n));return}var i=J,l=qs(),c=o.length===1?o[0].promise:o.length>1?Promise.all(o.map(_=>_.promise)):null;function d(_){l();try{a(_)}catch(g){(i.f&Qe)===0&&Wt(g,i)}pa()}if(r.length===0){c.then(()=>d(t.map(n)));return}var v=ro();function m(){Promise.all(r.map(_=>Ks(_))).then(_=>d([...t.map(n),..._])).catch(_=>Wt(_,i)).finally(()=>v())}c?c.then(()=>{l(),m(),pa()}):m()}function qs(){var e=J,t=ee,r=Ae,a=V;return function(o=!0){dt(e),ct(t),Er(r),o&&(e.f&Qe)===0&&(a==null||a.activate(),a==null||a.apply())}}function pa(e=!0){dt(null),ct(null),Er(null),e&&(V==null||V.deactivate())}function ro(){var e=J,t=e.b,r=V,a=t.is_rendered();return t.update_pending_count(1,r),r.increment(a,e),(n=!1)=>{t.update_pending_count(-1,r),r.decrement(a,e,n)}}function ta(e){var t=Le|Ie;return J!==null&&(J.f|=Ir),{ctx:Ae,deps:null,effects:null,equals:Wn,f:t,fn:e,reactions:null,rv:0,v:we,wv:0,parent:J,ac:null}}function Ks(e,t,r){let a=J;a===null&&hs();var n=void 0,o=Jt(we),i=!ee,l=new Map;return ii(()=>{var g;var c=J,d=Bn();n=d.promise;try{Promise.resolve(e()).then(d.resolve,d.reject).finally(pa)}catch(h){d.reject(h),pa()}var v=V;if(i){if((c.f&_r)!==0)var m=ro();if(a.b.is_rendered())(g=l.get(v))==null||g.reject(Tt),l.delete(v);else{for(const h of l.values())h.reject(Tt);l.clear()}l.set(v,d)}const _=(h,$=void 0)=>{if(m){var x=$===Tt;m(x)}if(!($===Tt||(c.f&Qe)!==0)){if(v.activate(),$)o.f|=Kt,Ar(o,$);else{(o.f&Kt)!==0&&(o.f^=Kt),Ar(o,h);for(const[f,S]of l){if(l.delete(f),f===v)break;S.reject(Tt)}}v.deactivate()}};d.promise.then(_,h=>_(null,h||"unknown"))}),ya(()=>{for(const c of l.values())c.reject(Tt)}),new Promise(c=>{function d(v){function m(){v===n?c(o):d(n)}v.then(m,m)}d(n)})}function de(e){const t=ta(e);return wo(t),t}function rn(e){const t=ta(e);return t.equals=qn,t}function Gs(e){var t=e.effects;if(t!==null){e.effects=null;for(var r=0;r<t.length;r+=1)Oe(t[r])}}function an(e){var t,r=J,a=e.parent;if(!Dt&&a!==null&&(a.f&(Qe|Fe))!==0)return Es(),e.v;dt(a);try{e.f&=~vr,Gs(e),t=Eo(e)}finally{dt(r)}return t}function ao(e){var t=an(e);if(!e.equals(t)&&(e.wv=ko(),(!(V!=null&&V.is_fork)||e.deps===null)&&(V!==null?V.capture(e,t,!0):e.v=t,e.deps===null))){_e(e,$e);return}Dt||(Pe!==null?(nn()||V!=null&&V.is_fork)&&Pe.set(e,t):Qa(e))}function Ys(e){var t,r;if(e.effects!==null)for(const a of e.effects)(a.teardown||a.ac)&&((t=a.teardown)==null||t.call(a),(r=a.ac)==null||r.abort(Tt),a.teardown=cr,a.ac=null,Ur(a,0),on(a))}function no(e){if(e.effects!==null)for(const t of e.effects)t.teardown&&Tr(t)}let Ba=new Set;const dr=new Map;let oo=!1;function Jt(e,t){var r={f:0,v:e,reactions:null,equals:Wn,rv:0,wv:0};return r}function F(e,t){const r=Jt(e);return wo(r),r}function Js(e,t=!1,r=!0){var n;const a=Jt(e);return t||(a.equals=qn),Qr&&r&&Ae!==null&&Ae.l!==null&&((n=Ae.l).s??(n.s=[])).push(a),a}function T(e,t,r=!1){ee!==null&&(!_t||(ee.f&Ca)!==0)&&ea()&&(ee.f&(Le|Et|Vr|Ca))!==0&&(lt===null||!kr.call(lt,e))&&ks();let a=r?ve(t):t;return Ar(e,a,ia)}function Ar(e,t,r=null){if(!e.equals(t)){dr.set(e,Dt?t:e.v);var a=pr.ensure();if(a.capture(e,t),(e.f&Le)!==0){const n=e;(e.f&Ie)!==0&&an(n),Pe===null&&Qa(n)}e.wv=ko(),so(e,Ie,r),ea()&&J!==null&&(J.f&$e)!==0&&(J.f&(xt|Gt))===0&&(at===null?di([e]):at.push(e)),!a.is_fork&&Ba.size>0&&!oo&&Zs()}return t}function Zs(){oo=!1;for(const e of Ba)(e.f&$e)!==0&&_e(e,At),na(e)&&Tr(e);Ba.clear()}function _n(e,t=1){var r=s(e),a=t===1?r++:r--;return T(e,r),a}function Br(e){T(e,e.v+1)}function so(e,t,r){var a=e.reactions;if(a!==null)for(var n=ea(),o=a.length,i=0;i<o;i++){var l=a[i],c=l.f;if(!(!n&&l===J)){var d=(c&Ie)===0;if(d&&_e(l,t),(c&Le)!==0){var v=l;Pe==null||Pe.delete(v),(c&vr)===0&&(c&it&&(l.f|=vr),so(v,At,r))}else if(d){var m=l;(c&Et)!==0&&pt!==null&&pt.add(m),r!==null?r.push(m):tn(m)}}}}function ve(e){if(typeof e!="object"||e===null||Ct in e)return e;const t=Ja(e);if(t!==ds&&t!==us)return e;var r=new Map,a=Ya(e),n=F(0),o=fr,i=l=>{if(fr===o)return l();var c=ee,d=fr;ct(null),yn(o);var v=l();return ct(c),yn(d),v};return a&&r.set("length",F(e.length)),new Proxy(e,{defineProperty(l,c,d){(!("value"in d)||d.configurable===!1||d.enumerable===!1||d.writable===!1)&&ws();var v=r.get(c);return v===void 0?i(()=>{var m=F(d.value);return r.set(c,m),m}):T(v,d.value,!0),!0},deleteProperty(l,c){var d=r.get(c);if(d===void 0){if(c in l){const v=i(()=>F(we));r.set(c,v),Br(n)}}else T(d,we),Br(n);return!0},get(l,c,d){var g;if(c===Ct)return e;var v=r.get(c),m=c in l;if(v===void 0&&(!m||(g=qt(l,c))!=null&&g.writable)&&(v=i(()=>{var h=ve(m?l[c]:we),$=F(h);return $}),r.set(c,v)),v!==void 0){var _=s(v);return _===we?void 0:_}return Reflect.get(l,c,d)},getOwnPropertyDescriptor(l,c){var d=Reflect.getOwnPropertyDescriptor(l,c);if(d&&"value"in d){var v=r.get(c);v&&(d.value=s(v))}else if(d===void 0){var m=r.get(c),_=m==null?void 0:m.v;if(m!==void 0&&_!==we)return{enumerable:!0,configurable:!0,value:_,writable:!0}}return d},has(l,c){var _;if(c===Ct)return!0;var d=r.get(c),v=d!==void 0&&d.v!==we||Reflect.has(l,c);if(d!==void 0||J!==null&&(!v||(_=qt(l,c))!=null&&_.writable)){d===void 0&&(d=i(()=>{var g=v?ve(l[c]):we,h=F(g);return h}),r.set(c,d));var m=s(d);if(m===we)return!1}return v},set(l,c,d,v){var N;var m=r.get(c),_=c in l;if(a&&c==="length")for(var g=d;g<m.v;g+=1){var h=r.get(g+"");h!==void 0?T(h,we):g in l&&(h=i(()=>F(we)),r.set(g+"",h))}if(m===void 0)(!_||(N=qt(l,c))!=null&&N.writable)&&(m=i(()=>F(void 0)),T(m,ve(d)),r.set(c,m));else{_=m.v!==we;var $=i(()=>ve(d));T(m,$)}var x=Reflect.getOwnPropertyDescriptor(l,c);if(x!=null&&x.set&&x.set.call(v,d),!_){if(a&&typeof c=="string"){var f=r.get("length"),S=Number(c);Number.isInteger(S)&&S>=f.v&&T(f,S+1)}Br(n)}return!0},ownKeys(l){s(n);var c=Reflect.ownKeys(l).filter(m=>{var _=r.get(m);return _===void 0||_.v!==we});for(var[d,v]of r)v.v!==we&&!(d in l)&&c.push(d);return c},setPrototypeOf(){$s()}})}function xn(e){try{if(e!==null&&typeof e=="object"&&Ct in e)return e[Ct]}catch{}return e}function Xs(e,t){return Object.is(xn(e),xn(t))}var bn,io,lo,co;function Qs(){if(bn===void 0){bn=window,io=/Firefox/.test(navigator.userAgent);var e=Element.prototype,t=Node.prototype,r=Text.prototype;lo=qt(t,"firstChild").get,co=qt(t,"nextSibling").get,fn(e)&&(e.__click=void 0,e.__className=void 0,e.__attributes=null,e.__style=void 0,e.__e=void 0),fn(r)&&(r.__t=void 0)}}function Rt(e=""){return document.createTextNode(e)}function Nr(e){return lo.call(e)}function ra(e){return co.call(e)}function u(e,t){return Nr(e)}function te(e,t=!1){{var r=Nr(e);return r instanceof Comment&&r.data===""?ra(r):r}}function b(e,t=1,r=!1){let a=e;for(;t--;)a=ra(a);return a}function ei(e){e.textContent=""}function uo(){return!1}function fo(e,t,r){return document.createElementNS(t??jn,e,void 0)}function ti(e,t){if(t){const r=document.body;e.autofocus=!0,Lt(()=>{document.activeElement===r&&e.focus()})}}let mn=!1;function ri(){mn||(mn=!0,document.addEventListener("reset",e=>{Promise.resolve().then(()=>{var t;if(!e.defaultPrevented)for(const r of e.target.elements)(t=r.__on_r)==null||t.call(r)})},{capture:!0}))}function ga(e){var t=ee,r=J;ct(null),dt(null);try{return e()}finally{ct(t),dt(r)}}function ai(e,t,r,a=r){e.addEventListener(t,()=>ga(r));const n=e.__on_r;n?e.__on_r=()=>{n(),a(!0)}:e.__on_r=()=>a(!0),ri()}function vo(e){J===null&&(ee===null&&ms(),bs()),Dt&&xs()}function ni(e,t){var r=t.last;r===null?t.last=t.first=e:(r.next=e,e.prev=r,t.last=e)}function bt(e,t){var r=J;r!==null&&(r.f&Fe)!==0&&(e|=Fe);var a={ctx:Ae,deps:null,nodes:null,f:e|Ie|it,first:null,fn:t,last:null,next:null,parent:r,b:r&&r.b,prev:null,teardown:null,wv:0,ac:null};V==null||V.register_created_effect(a);var n=a;if((e&Sr)!==0)br!==null?br.push(a):pr.ensure().schedule(a);else if(t!==null){try{Tr(a)}catch(i){throw Oe(a),i}n.deps===null&&n.teardown===null&&n.nodes===null&&n.first===n.last&&(n.f&Ir)===0&&(n=n.first,(e&Et)!==0&&(e&Yt)!==0&&n!==null&&(n.f|=Yt))}if(n!==null&&(n.parent=r,r!==null&&ni(n,r),ee!==null&&(ee.f&Le)!==0&&(e&Gt)===0)){var o=ee;(o.effects??(o.effects=[])).push(n)}return a}function nn(){return ee!==null&&!_t}function ya(e){const t=bt(Xr,null);return _e(t,$e),t.teardown=e,t}function Be(e){vo();var t=J.f,r=!ee&&(t&xt)!==0&&(t&_r)===0;if(r){var a=Ae;(a.e??(a.e=[])).push(e)}else return po(e)}function po(e){return bt(Sr|Vn,e)}function oi(e){return vo(),bt(Xr|Vn,e)}function si(e){pr.ensure();const t=bt(Gt|Ir,e);return(r={})=>new Promise(a=>{r.outro?ur(t,()=>{Oe(t),a(void 0)}):(Oe(t),a(void 0))})}function ho(e){return bt(Sr,e)}function ii(e){return bt(Vr|Ir,e)}function _o(e,t=0){return bt(Xr|t,e)}function j(e,t=[],r=[],a=[]){to(a,t,r,n=>{bt(Xr,()=>e(...n.map(s)))})}function aa(e,t=0){var r=bt(Et|t,e);return r}function xo(e,t=0){var r=bt(Za|t,e);return r}function qe(e){return bt(xt|Ir,e)}function bo(e){var t=e.teardown;if(t!==null){const r=Dt,a=ee;gn(!0),ct(null);try{t.call(null)}finally{gn(r),ct(a)}}}function on(e,t=!1){var r=e.first;for(e.first=e.last=null;r!==null;){const n=r.ac;n!==null&&ga(()=>{n.abort(Tt)});var a=r.next;(r.f&Gt)!==0?r.parent=null:Oe(r,t),r=a}}function li(e){for(var t=e.first;t!==null;){var r=t.next;(t.f&xt)===0&&Oe(t),t=r}}function Oe(e,t=!0){var r=!1;(t||(e.f&vs)!==0)&&e.nodes!==null&&e.nodes.end!==null&&(ci(e.nodes.start,e.nodes.end),r=!0),_e(e,vn),on(e,t&&!r),Ur(e,0);var a=e.nodes&&e.nodes.t;if(a!==null)for(const o of a)o.stop();bo(e),e.f^=vn,e.f|=Qe;var n=e.parent;n!==null&&n.first!==null&&mo(e),e.next=e.prev=e.teardown=e.ctx=e.deps=e.fn=e.nodes=e.ac=e.b=null}function ci(e,t){for(;e!==null;){var r=e===t?null:ra(e);e.remove(),e=r}}function mo(e){var t=e.parent,r=e.prev,a=e.next;r!==null&&(r.next=a),a!==null&&(a.prev=r),t!==null&&(t.first===e&&(t.first=a),t.last===e&&(t.last=r))}function ur(e,t,r=!0){var a=[];go(e,a,!0);var n=()=>{r&&Oe(e),t&&t()},o=a.length;if(o>0){var i=()=>--o||n();for(var l of a)l.out(i)}else n()}function go(e,t,r){if((e.f&Fe)===0){e.f^=Fe;var a=e.nodes&&e.nodes.t;if(a!==null)for(const l of a)(l.is_global||r)&&t.push(l);for(var n=e.first;n!==null;){var o=n.next;if((n.f&Gt)===0){var i=(n.f&Yt)!==0||(n.f&xt)!==0&&(e.f&Et)!==0;go(n,t,i?r:!1)}n=o}}}function sn(e){yo(e,!0)}function yo(e,t){if((e.f&Fe)!==0){e.f^=Fe,(e.f&$e)===0&&(_e(e,Ie),pr.ensure().schedule(e));for(var r=e.first;r!==null;){var a=r.next,n=(r.f&Yt)!==0||(r.f&xt)!==0;yo(r,n?t:!1),r=a}var o=e.nodes&&e.nodes.t;if(o!==null)for(const i of o)(i.is_global||t)&&i.in()}}function ln(e,t){if(e.nodes)for(var r=e.nodes.start,a=e.nodes.end;r!==null;){var n=r===a?null:ra(r);t.append(r),r=n}}let va=!1,Dt=!1;function gn(e){Dt=e}let ee=null,_t=!1;function ct(e){ee=e}let J=null;function dt(e){J=e}let lt=null;function wo(e){ee!==null&&(lt===null?lt=[e]:lt.push(e))}let We=null,Ye=0,at=null;function di(e){at=e}let $o=1,rr=0,fr=rr;function yn(e){fr=e}function ko(){return++$o}function na(e){var t=e.f;if((t&Ie)!==0)return!0;if(t&Le&&(e.f&=~vr),(t&At)!==0){for(var r=e.deps,a=r.length,n=0;n<a;n++){var o=r[n];if(na(o)&&ao(o),o.wv>e.wv)return!0}(t&it)!==0&&Pe===null&&_e(e,$e)}return!1}function So(e,t,r=!0){var a=e.reactions;if(a!==null&&!(lt!==null&&kr.call(lt,e)))for(var n=0;n<a.length;n++){var o=a[n];(o.f&Le)!==0?So(o,t,!1):t===o&&(r?_e(o,Ie):(o.f&$e)!==0&&_e(o,At),tn(o))}}function Eo(e){var $;var t=We,r=Ye,a=at,n=ee,o=lt,i=Ae,l=_t,c=fr,d=e.f;We=null,Ye=0,at=null,ee=(d&(xt|Gt))===0?e:null,lt=null,Er(e.ctx),_t=!1,fr=++rr,e.ac!==null&&(ga(()=>{e.ac.abort(Tt)}),e.ac=null);try{e.f|=La;var v=e.fn,m=v();e.f|=_r;var _=e.deps,g=V==null?void 0:V.is_fork;if(We!==null){var h;if(g||Ur(e,Ye),_!==null&&Ye>0)for(_.length=Ye+We.length,h=0;h<We.length;h++)_[Ye+h]=We[h];else e.deps=_=We;if(nn()&&(e.f&it)!==0)for(h=Ye;h<_.length;h++)(($=_[h]).reactions??($.reactions=[])).push(e)}else!g&&_!==null&&Ye<_.length&&(Ur(e,Ye),_.length=Ye);if(ea()&&at!==null&&!_t&&_!==null&&(e.f&(Le|At|Ie))===0)for(h=0;h<at.length;h++)So(at[h],e);if(n!==null&&n!==e){if(rr++,n.deps!==null)for(let x=0;x<r;x+=1)n.deps[x].rv=rr;if(t!==null)for(const x of t)x.rv=rr;at!==null&&(a===null?a=at:a.push(...at))}return(e.f&Kt)!==0&&(e.f^=Kt),m}catch(x){return Gn(x)}finally{e.f^=La,We=t,Ye=r,at=a,ee=n,lt=o,Er(i),_t=l,fr=c}}function ui(e,t){let r=t.reactions;if(r!==null){var a=ls.call(r,e);if(a!==-1){var n=r.length-1;n===0?r=t.reactions=null:(r[a]=r[n],r.pop())}}if(r===null&&(t.f&Le)!==0&&(We===null||!kr.call(We,t))){var o=t;(o.f&it)!==0&&(o.f^=it,o.f&=~vr),o.v!==we&&Qa(o),Ys(o),Ur(o,0)}}function Ur(e,t){var r=e.deps;if(r!==null)for(var a=t;a<r.length;a++)ui(e,r[a])}function Tr(e){var t=e.f;if((t&Qe)===0){_e(e,$e);var r=J,a=va;J=e,va=!0;try{(t&(Et|Za))!==0?li(e):on(e),bo(e);var n=Eo(e);e.teardown=typeof n=="function"?n:null,e.wv=$o;var o;is&&Ms&&(e.f&Ie)!==0&&e.deps}finally{va=a,J=r}}}async function fi(){await Promise.resolve(),Ds()}function s(e){var t=e.f,r=(t&Le)!==0;if(ee!==null&&!_t){var a=J!==null&&(J.f&Qe)!==0;if(!a&&(lt===null||!kr.call(lt,e))){var n=ee.deps;if((ee.f&La)!==0)e.rv<rr&&(e.rv=rr,We===null&&n!==null&&n[Ye]===e?Ye++:We===null?We=[e]:We.push(e));else{(ee.deps??(ee.deps=[])).push(e);var o=e.reactions;o===null?e.reactions=[ee]:kr.call(o,ee)||o.push(ee)}}}if(Dt&&dr.has(e))return dr.get(e);if(r){var i=e;if(Dt){var l=i.v;return((i.f&$e)===0&&i.reactions!==null||No(i))&&(l=an(i)),dr.set(i,l),l}var c=(i.f&it)===0&&!_t&&ee!==null&&(va||(ee.f&it)!==0),d=(i.f&_r)===0;na(i)&&(c&&(i.f|=it),ao(i)),c&&!d&&(no(i),Ao(i))}if(Pe!=null&&Pe.has(e))return Pe.get(e);if((e.f&Kt)!==0)throw e.v;return e.v}function Ao(e){if(e.f|=it,e.deps!==null)for(const t of e.deps)(t.reactions??(t.reactions=[])).push(e),(t.f&Le)!==0&&(t.f&it)===0&&(no(t),Ao(t))}function No(e){if(e.v===we)return!0;if(e.deps===null)return!1;for(const t of e.deps)if(dr.has(t)||(t.f&Le)!==0&&No(t))return!0;return!1}function Mr(e){var t=_t;try{return _t=!0,e()}finally{_t=t}}function er(e){if(!(typeof e!="object"||!e||e instanceof EventTarget)){if(Ct in e)Ha(e);else if(!Array.isArray(e))for(let t in e){const r=e[t];typeof r=="object"&&r&&Ct in r&&Ha(r)}}}function Ha(e,t=new Set){if(typeof e=="object"&&e!==null&&!(e instanceof EventTarget)&&!t.has(e)){t.add(e),e instanceof Date&&e.getTime();for(let a in e)try{Ha(e[a],t)}catch{}const r=Ja(e);if(r!==Object.prototype&&r!==Array.prototype&&r!==Map.prototype&&r!==Set.prototype&&r!==Date.prototype){const a=Fn(r);for(let n in a){const o=a[n].get;if(o)try{o.call(e)}catch{}}}}}const ar=Symbol("events"),To=new Set,Va=new Set;function Mo(e,t,r,a={}){function n(o){if(a.capture||Ua.call(t,o),!o.cancelBubble)return ga(()=>r==null?void 0:r.call(this,o))}return e.startsWith("pointer")||e.startsWith("touch")||e==="wheel"?Lt(()=>{t.addEventListener(e,n,a)}):t.addEventListener(e,n,a),n}function Po(e,t,r,a,n){var o={capture:a,passive:n},i=Mo(e,t,r,o);(t===document.body||t===window||t===document||t instanceof HTMLMediaElement)&&ya(()=>{t.removeEventListener(e,i,o)})}function re(e,t,r){(t[ar]??(t[ar]={}))[e]=r}function zt(e){for(var t=0;t<e.length;t++)To.add(e[t]);for(var r of Va)r(e)}let wn=null;function Ua(e){var x,f;var t=this,r=t.ownerDocument,a=e.type,n=((x=e.composedPath)==null?void 0:x.call(e))||[],o=n[0]||e.target;wn=e;var i=0,l=wn===e&&e[ar];if(l){var c=n.indexOf(l);if(c!==-1&&(t===document||t===window)){e[ar]=t;return}var d=n.indexOf(t);if(d===-1)return;c<=d&&(i=c)}if(o=n[i]||e.target,o!==t){cs(e,"currentTarget",{configurable:!0,get(){return o||r}});var v=ee,m=J;ct(null),dt(null);try{for(var _,g=[];o!==null;){var h=o.assignedSlot||o.parentNode||o.host||null;try{var $=(f=o[ar])==null?void 0:f[a];$!=null&&(!o.disabled||e.target===o)&&$.call(o,e)}catch(S){_?g.push(S):_=S}if(e.cancelBubble||h===t||h===null)break;o=h}if(_){for(let S of g)queueMicrotask(()=>{throw S});throw _}}finally{e[ar]=t,delete e.currentTarget,ct(v),dt(m)}}}var Rn;const Aa=((Rn=globalThis==null?void 0:globalThis.window)==null?void 0:Rn.trustedTypes)&&globalThis.window.trustedTypes.createPolicy("svelte-trusted-html",{createHTML:e=>e});function vi(e){return(Aa==null?void 0:Aa.createHTML(e))??e}function Io(e){var t=fo("template");return t.innerHTML=vi(e.replaceAll("<!>","<!---->")),t.content}function Wr(e,t){var r=J;r.nodes===null&&(r.nodes={start:e,end:t,a:null,t:null})}function E(e,t){var r=(t&as)!==0,a=(t&ns)!==0,n,o=!e.startsWith("<!>");return()=>{n===void 0&&(n=Io(o?e:"<!>"+e),r||(n=Nr(n)));var i=a||io?document.importNode(n,!0):n.cloneNode(!0);if(r){var l=Nr(i),c=i.lastChild;Wr(l,c)}else Wr(i,i);return i}}function pi(e,t,r="svg"){var a=!e.startsWith("<!>"),n=`<${r}>${a?e:"<!>"+e}</${r}>`,o;return()=>{if(!o){var i=Io(n),l=Nr(i);o=Nr(l)}var c=o.cloneNode(!0);return Wr(c,c),c}}function hi(e,t){return pi(e,t,"svg")}function ce(){var e=document.createDocumentFragment(),t=document.createComment(""),r=Rt();return e.append(t,r),Wr(t,r),e}function y(e,t){e!==null&&e.before(t)}function _i(e){return e.endsWith("capture")&&e!=="gotpointercapture"&&e!=="lostpointercapture"}const xi=["beforeinput","click","change","dblclick","contextmenu","focusin","focusout","input","keydown","keyup","mousedown","mousemove","mouseout","mouseover","mouseup","pointerdown","pointermove","pointerout","pointerover","pointerup","touchend","touchmove","touchstart"];function bi(e){return xi.includes(e)}const mi={formnovalidate:"formNoValidate",ismap:"isMap",nomodule:"noModule",playsinline:"playsInline",readonly:"readOnly",defaultvalue:"defaultValue",defaultchecked:"defaultChecked",srcobject:"srcObject",novalidate:"noValidate",allowfullscreen:"allowFullscreen",disablepictureinpicture:"disablePictureInPicture",disableremoteplayback:"disableRemotePlayback"};function gi(e){return e=e.toLowerCase(),mi[e]??e}const yi=["touchstart","touchmove"];function wi(e){return yi.includes(e)}function P(e,t){var r=t==null?"":typeof t=="object"?`${t}`:t;r!==(e.__t??(e.__t=e.nodeValue))&&(e.__t=r,e.nodeValue=`${r}`)}function $i(e,t){return ki(e,t)}const sa=new Map;function ki(e,{target:t,anchor:r,props:a={},events:n,context:o,intro:i=!0,transformError:l}){Qs();var c=void 0,d=si(()=>{var v=r??t.appendChild(Rt());Bs(v,{pending:()=>{}},g=>{He({});var h=Ae;o&&(h.c=o),n&&(a.$$events=n),c=e(g,a)||{},Ve()},l);var m=new Set,_=g=>{for(var h=0;h<g.length;h++){var $=g[h];if(!m.has($)){m.add($);var x=wi($);for(const N of[t,document]){var f=sa.get(N);f===void 0&&(f=new Map,sa.set(N,f));var S=f.get($);S===void 0?(N.addEventListener($,Ua,{passive:x}),f.set($,1)):f.set($,S+1)}}}};return _(ma(To)),Va.add(_),()=>{var x;for(var g of m)for(const f of[t,document]){var h=sa.get(f),$=h.get(g);--$==0?(f.removeEventListener(g,Ua),h.delete(g),h.size===0&&sa.delete(f)):h.set(g,$)}Va.delete(_),v!==r&&((x=v.parentNode)==null||x.removeChild(v))}});return Si.set(c,d),c}let Si=new WeakMap;var ht,kt,Xe,lr,Jr,Zr,ba;class cn{constructor(t,r=!0){vt(this,"anchor");K(this,ht,new Map);K(this,kt,new Map);K(this,Xe,new Map);K(this,lr,new Set);K(this,Jr,!0);K(this,Zr,t=>{if(p(this,ht).has(t)){var r=p(this,ht).get(t),a=p(this,kt).get(r);if(a)sn(a),p(this,lr).delete(r);else{var n=p(this,Xe).get(r);n&&(p(this,kt).set(r,n.effect),p(this,Xe).delete(r),n.fragment.lastChild.remove(),this.anchor.before(n.fragment),a=n.effect)}for(const[o,i]of p(this,ht)){if(p(this,ht).delete(o),o===t)break;const l=p(this,Xe).get(i);l&&(Oe(l.effect),p(this,Xe).delete(i))}for(const[o,i]of p(this,kt)){if(o===r||p(this,lr).has(o))continue;const l=()=>{if(Array.from(p(this,ht).values()).includes(o)){var d=document.createDocumentFragment();ln(i,d),d.append(Rt()),p(this,Xe).set(o,{effect:i,fragment:d})}else Oe(i);p(this,lr).delete(o),p(this,kt).delete(o)};p(this,Jr)||!a?(p(this,lr).add(o),ur(i,l,!1)):l()}}});K(this,ba,t=>{p(this,ht).delete(t);const r=Array.from(p(this,ht).values());for(const[a,n]of p(this,Xe))r.includes(a)||(Oe(n.effect),p(this,Xe).delete(a))});this.anchor=t,Q(this,Jr,r)}ensure(t,r){var a=V,n=uo();if(r&&!p(this,kt).has(t)&&!p(this,Xe).has(t))if(n){var o=document.createDocumentFragment(),i=Rt();o.append(i),p(this,Xe).set(t,{effect:qe(()=>r(i)),fragment:o})}else p(this,kt).set(t,qe(()=>r(this.anchor)));if(p(this,ht).set(a,t),n){for(const[l,c]of p(this,kt))l===t?a.unskip_effect(c):a.skip_effect(c);for(const[l,c]of p(this,Xe))l===t?a.unskip_effect(c.effect):a.skip_effect(c.effect);a.oncommit(p(this,Zr)),a.ondiscard(p(this,ba))}else p(this,Zr).call(this,a)}}ht=new WeakMap,kt=new WeakMap,Xe=new WeakMap,lr=new WeakMap,Jr=new WeakMap,Zr=new WeakMap,ba=new WeakMap;function q(e,t,r=!1){var a=new cn(e),n=r?Yt:0;function o(i,l){a.ensure(i,l)}aa(()=>{var i=!1;t((l,c=0)=>{i=!0,o(c,l)}),i||o(-1,null)},n)}function Se(e,t){return t}function Ei(e,t,r){for(var a=[],n=t.length,o,i=t.length,l=0;l<n;l++){let m=t[l];ur(m,()=>{if(o){if(o.pending.delete(m),o.done.add(m),o.pending.size===0){var _=e.outrogroups;Wa(e,ma(o.done)),_.delete(o),_.size===0&&(e.outrogroups=null)}}else i-=1},!1)}if(i===0){var c=a.length===0&&r!==null;if(c){var d=r,v=d.parentNode;ei(v),v.append(d),e.items.clear()}Wa(e,t,!c)}else o={pending:new Set(t),done:new Set},(e.outrogroups??(e.outrogroups=new Set)).add(o)}function Wa(e,t,r=!0){var a;if(e.pending.size>0){a=new Set;for(const i of e.pending.values())for(const l of i)a.add(e.items.get(l).e)}for(var n=0;n<t.length;n++){var o=t[n];if(a!=null&&a.has(o)){o.f|=St;const i=document.createDocumentFragment();ln(o,i)}else Oe(t[n],r)}}var $n;function Ee(e,t,r,a,n,o=null){var i=e,l=new Map,c=(t&Dn)!==0;if(c){var d=e;i=d.appendChild(Rt())}var v=null,m=rn(()=>{var N=r();return Ya(N)?N:N==null?[]:ma(N)}),_,g=new Map,h=!0;function $(N){(S.effect.f&Qe)===0&&(S.pending.delete(N),S.fallback=v,Ai(S,_,i,t,a),v!==null&&(_.length===0?(v.f&St)===0?sn(v):(v.f^=St,jr(v,null,i)):ur(v,()=>{v=null})))}function x(N){S.pending.delete(N)}var f=aa(()=>{_=s(m);for(var N=_.length,k=new Set,M=V,C=uo(),I=0;I<N;I+=1){var D=_[I],O=a(D,I),H=h?null:l.get(O);H?(H.v&&Ar(H.v,D),H.i&&Ar(H.i,I),C&&M.unskip_effect(H.e)):(H=Ni(l,h?i:$n??($n=Rt()),D,O,I,n,t,r),h||(H.e.f|=St),l.set(O,H)),k.add(O)}if(N===0&&o&&!v&&(h?v=qe(()=>o(i)):(v=qe(()=>o($n??($n=Rt()))),v.f|=St)),N>k.size&&_s(),!h)if(g.set(M,k),C){for(const[z,U]of l)k.has(z)||M.skip_effect(U.e);M.oncommit($),M.ondiscard(x)}else $(M);s(m)}),S={effect:f,items:l,pending:g,outrogroups:null,fallback:v};h=!1}function Lr(e){for(;e!==null&&(e.f&xt)===0;)e=e.next;return e}function Ai(e,t,r,a,n){var H,z,U,G,B,w,L,W,A;var o=(a&Zo)!==0,i=t.length,l=e.items,c=Lr(e.effect.first),d,v=null,m,_=[],g=[],h,$,x,f;if(o)for(f=0;f<i;f+=1)h=t[f],$=n(h,f),x=l.get($).e,(x.f&St)===0&&((z=(H=x.nodes)==null?void 0:H.a)==null||z.measure(),(m??(m=new Set)).add(x));for(f=0;f<i;f+=1){if(h=t[f],$=n(h,f),x=l.get($).e,e.outrogroups!==null)for(const R of e.outrogroups)R.pending.delete(x),R.done.delete(x);if((x.f&Fe)!==0&&(sn(x),o&&((G=(U=x.nodes)==null?void 0:U.a)==null||G.unfix(),(m??(m=new Set)).delete(x))),(x.f&St)!==0)if(x.f^=St,x===c)jr(x,null,r);else{var S=v?v.next:c;x===e.effect.last&&(e.effect.last=x.prev),x.prev&&(x.prev.next=x.next),x.next&&(x.next.prev=x.prev),Bt(e,v,x),Bt(e,x,S),jr(x,S,r),v=x,_=[],g=[],c=Lr(v.next);continue}if(x!==c){if(d!==void 0&&d.has(x)){if(_.length<g.length){var N=g[0],k;v=N.prev;var M=_[0],C=_[_.length-1];for(k=0;k<_.length;k+=1)jr(_[k],N,r);for(k=0;k<g.length;k+=1)d.delete(g[k]);Bt(e,M.prev,C.next),Bt(e,v,M),Bt(e,C,N),c=N,v=C,f-=1,_=[],g=[]}else d.delete(x),jr(x,c,r),Bt(e,x.prev,x.next),Bt(e,x,v===null?e.effect.first:v.next),Bt(e,v,x),v=x;continue}for(_=[],g=[];c!==null&&c!==x;)(d??(d=new Set)).add(c),g.push(c),c=Lr(c.next);if(c===null)continue}(x.f&St)===0&&_.push(x),v=x,c=Lr(x.next)}if(e.outrogroups!==null){for(const R of e.outrogroups)R.pending.size===0&&(Wa(e,ma(R.done)),(B=e.outrogroups)==null||B.delete(R));e.outrogroups.size===0&&(e.outrogroups=null)}if(c!==null||d!==void 0){var I=[];if(d!==void 0)for(x of d)(x.f&Fe)===0&&I.push(x);for(;c!==null;)(c.f&Fe)===0&&c!==e.fallback&&I.push(c),c=Lr(c.next);var D=I.length;if(D>0){var O=(a&Dn)!==0&&i===0?r:null;if(o){for(f=0;f<D;f+=1)(L=(w=I[f].nodes)==null?void 0:w.a)==null||L.measure();for(f=0;f<D;f+=1)(A=(W=I[f].nodes)==null?void 0:W.a)==null||A.fix()}Ei(e,I,O)}}o&&Lt(()=>{var R,Z;if(m!==void 0)for(x of m)(Z=(R=x.nodes)==null?void 0:R.a)==null||Z.apply()})}function Ni(e,t,r,a,n,o,i,l){var c=(i&Yo)!==0?(i&Xo)===0?Js(r,!1,!1):Jt(r):null,d=(i&Jo)!==0?Jt(n):null;return{v:c,i:d,e:qe(()=>(o(t,c??r,d??n,l),()=>{e.delete(a)}))}}function jr(e,t,r){if(e.nodes)for(var a=e.nodes.start,n=e.nodes.end,o=t&&(t.f&St)===0?t.nodes.start:r;a!==null;){var i=ra(a);if(o.before(a),a===n)return;a=i}}function Bt(e,t,r){t===null?e.effect.first=r:t.next=r,r===null?e.effect.last=t:r.prev=t}function ke(e,t,r,a,n){var l;var o=(l=t.$$slots)==null?void 0:l[r],i=!1;o===!0&&(o=t.children,i=!0),o===void 0||o(e,i?()=>a:a)}function Ti(e,t,r){var a=new cn(e);aa(()=>{var n=t()??null;a.ensure(n,n&&(o=>r(o,n)))},Yt)}function Mi(e,t,r,a,n,o){var i=null,l=e,c=new cn(l,!1);aa(()=>{const d=t()||null;var v=os;if(d===null){c.ensure(null,null);return}return c.ensure(d,m=>{if(d){if(i=fo(d,v),Wr(i,i),a){var _=i.appendChild(Rt());a(i,_)}J.nodes.end=i,m.before(i)}}),()=>{}},Yt),ya(()=>{})}function Pi(e,t){var r=void 0,a;xo(()=>{r!==(r=t())&&(a&&(Oe(a),a=null),r&&(a=qe(()=>{ho(()=>r(e))})))})}function Oo(e){var t,r,a="";if(typeof e=="string"||typeof e=="number")a+=e;else if(typeof e=="object")if(Array.isArray(e)){var n=e.length;for(t=0;t<n;t++)e[t]&&(r=Oo(e[t]))&&(a&&(a+=" "),a+=r)}else for(r in e)e[r]&&(a&&(a+=" "),a+=r);return a}function Ii(){for(var e,t,r=0,a="",n=arguments.length;r<n;r++)(e=arguments[r])&&(t=Oo(e))&&(a&&(a+=" "),a+=t);return a}function Oi(e){return typeof e=="object"?Ii(e):e??""}const kn=[...` 	
\r\f \v\uFEFF`];function Ci(e,t,r){var a=e==null?"":""+e;if(r){for(var n of Object.keys(r))if(r[n])a=a?a+" "+n:n;else if(a.length)for(var o=n.length,i=0;(i=a.indexOf(n,i))>=0;){var l=i+o;(i===0||kn.includes(a[i-1]))&&(l===a.length||kn.includes(a[l]))?a=(i===0?"":a.substring(0,i))+a.substring(l+1):i=l}}return a===""?null:a}function Sn(e,t=!1){var r=t?" !important;":";",a="";for(var n of Object.keys(e)){var o=e[n];o!=null&&o!==""&&(a+=" "+n+": "+o+r)}return a}function Na(e){return e[0]!=="-"||e[1]!=="-"?e.toLowerCase():e}function Li(e,t){if(t){var r="",a,n;if(Array.isArray(t)?(a=t[0],n=t[1]):a=t,e){e=String(e).replaceAll(/\s*\/\*.*?\*\/\s*/g,"").trim();var o=!1,i=0,l=!1,c=[];a&&c.push(...Object.keys(a).map(Na)),n&&c.push(...Object.keys(n).map(Na));var d=0,v=-1;const $=e.length;for(var m=0;m<$;m++){var _=e[m];if(l?_==="/"&&e[m-1]==="*"&&(l=!1):o?o===_&&(o=!1):_==="/"&&e[m+1]==="*"?l=!0:_==='"'||_==="'"?o=_:_==="("?i++:_===")"&&i--,!l&&o===!1&&i===0){if(_===":"&&v===-1)v=m;else if(_===";"||m===$-1){if(v!==-1){var g=Na(e.substring(d,v).trim());if(!c.includes(g)){_!==";"&&m++;var h=e.substring(d,m).trim();r+=" "+h+";"}}d=m+1,v=-1}}}}return a&&(r+=Sn(a)),n&&(r+=Sn(n,!0)),r=r.trim(),r===""?null:r}return e==null?null:String(e)}function xe(e,t,r,a,n,o){var i=e.__className;if(i!==r||i===void 0){var l=Ci(r,a,o);l==null?e.removeAttribute("class"):t?e.className=l:e.setAttribute("class",l),e.__className=r}else if(o&&n!==o)for(var c in o){var d=!!o[c];(n==null||d!==!!n[c])&&e.classList.toggle(c,d)}return o}function Ta(e,t={},r,a){for(var n in r){var o=r[n];t[n]!==o&&(r[n]==null?e.style.removeProperty(n):e.style.setProperty(n,o,a))}}function Ri(e,t,r,a){var n=e.__style;if(n!==t){var o=Li(t,a);o==null?e.removeAttribute("style"):e.style.cssText=o,e.__style=t}else a&&(Array.isArray(a)?(Ta(e,r==null?void 0:r[0],a[0]),Ta(e,r==null?void 0:r[1],a[1],"important")):Ta(e,r,a));return a}function qa(e,t,r=!1){if(e.multiple){if(t==null)return;if(!Ya(t))return As();for(var a of e.options)a.selected=t.includes(En(a));return}for(a of e.options){var n=En(a);if(Xs(n,t)){a.selected=!0;return}}(!r||t!==void 0)&&(e.selectedIndex=-1)}function Di(e){var t=new MutationObserver(()=>{qa(e,e.__value)});t.observe(e,{childList:!0,subtree:!0,attributes:!0,attributeFilter:["value"]}),ya(()=>{t.disconnect()})}function En(e){return"__value"in e?e.__value:e.value}const Rr=Symbol("class"),Dr=Symbol("style"),Co=Symbol("is custom element"),Lo=Symbol("is html"),zi=Xa?"option":"OPTION",ji=Xa?"select":"SELECT",Fi=Xa?"progress":"PROGRESS";function zr(e,t){var r=wa(e);r.value===(r.value=t??void 0)||e.value===t&&(t!==0||e.nodeName!==Fi)||(e.value=t??"")}function An(e,t){var r=wa(e);r.checked!==(r.checked=t??void 0)&&(e.checked=t)}function Bi(e,t){t?e.hasAttribute("selected")||e.setAttribute("selected",""):e.removeAttribute("selected")}function Ka(e,t,r,a){var n=wa(e);n[t]!==(n[t]=r)&&(t==="loading"&&(e[ps]=r),r==null?e.removeAttribute(t):typeof r!="string"&&Ro(e).includes(t)?e[t]=r:e.setAttribute(t,r))}function Hi(e,t,r,a,n=!1,o=!1){var i=wa(e),l=i[Co],c=!i[Lo],d=t||{},v=e.nodeName===zi;for(var m in t)m in r||(r[m]=null);r.class?r.class=Oi(r.class):r[Rr]&&(r.class=null),r[Dr]&&(r.style??(r.style=null));var _=Ro(e);for(const k in r){let M=r[k];if(v&&k==="value"&&M==null){e.value=e.__value="",d[k]=M;continue}if(k==="class"){var g=e.namespaceURI==="http://www.w3.org/1999/xhtml";xe(e,g,M,a,t==null?void 0:t[Rr],r[Rr]),d[k]=M,d[Rr]=r[Rr];continue}if(k==="style"){Ri(e,M,t==null?void 0:t[Dr],r[Dr]),d[k]=M,d[Dr]=r[Dr];continue}var h=d[k];if(!(M===h&&!(M===void 0&&e.hasAttribute(k)))){d[k]=M;var $=k[0]+k[1];if($!=="$$")if($==="on"){const C={},I="$$"+k;let D=k.slice(2);var x=bi(D);if(_i(D)&&(D=D.slice(0,-7),C.capture=!0),!x&&h){if(M!=null)continue;e.removeEventListener(D,d[I],C),d[I]=null}if(x)re(D,e,M),zt([D]);else if(M!=null){let O=function(H){d[k].call(this,H)};var N=O;d[I]=Mo(D,e,O,C)}}else if(k==="style")Ka(e,k,M);else if(k==="autofocus")ti(e,!!M);else if(!l&&(k==="__value"||k==="value"&&M!=null))e.value=e.__value=M;else if(k==="selected"&&v)Bi(e,M);else{var f=k;c||(f=gi(f));var S=f==="defaultValue"||f==="defaultChecked";if(M==null&&!l&&!S)if(i[k]=null,f==="value"||f==="checked"){let C=e;const I=t===void 0;if(f==="value"){let D=C.defaultValue;C.removeAttribute(f),C.defaultValue=D,C.value=C.__value=I?D:null}else{let D=C.defaultChecked;C.removeAttribute(f),C.defaultChecked=D,C.checked=I?D:!1}}else e.removeAttribute(k);else S||_.includes(f)&&(l||typeof M!="string")?(e[f]=M,f in i&&(i[f]=we)):typeof M!="function"&&Ka(e,f,M)}}}return d}function Nn(e,t,r=[],a=[],n=[],o,i=!1,l=!1){to(n,r,a,c=>{var d=void 0,v={},m=e.nodeName===ji,_=!1;if(xo(()=>{var h=t(...c.map(s)),$=Hi(e,d,h,o,i,l);_&&m&&"value"in h&&qa(e,h.value);for(let f of Object.getOwnPropertySymbols(v))h[f]||Oe(v[f]);for(let f of Object.getOwnPropertySymbols(h)){var x=h[f];f.description===ss&&(!d||x!==d[f])&&(v[f]&&Oe(v[f]),v[f]=qe(()=>Pi(e,()=>x))),$[f]=x}d=$}),m){var g=e;ho(()=>{qa(g,d.value,!0),Di(g)})}_=!0})}function wa(e){return e.__attributes??(e.__attributes={[Co]:e.nodeName.includes("-"),[Lo]:e.namespaceURI===jn})}var Tn=new Map;function Ro(e){var t=e.getAttribute("is")||e.nodeName,r=Tn.get(t);if(r)return r;Tn.set(t,r=[]);for(var a,n=e,o=Element.prototype;o!==n;){a=Fn(n);for(var i in a)a[i].set&&r.push(i);n=Ja(n)}return r}function Pr(e,t,r=t){var a=new WeakSet;ai(e,"input",async n=>{var o=n?e.defaultValue:e.value;if(o=Ma(e)?Pa(o):o,r(o),V!==null&&a.add(V),await fi(),o!==(o=t())){var i=e.selectionStart,l=e.selectionEnd,c=e.value.length;if(e.value=o??"",l!==null){var d=e.value.length;i===l&&l===c&&d>c?(e.selectionStart=d,e.selectionEnd=d):(e.selectionStart=i,e.selectionEnd=Math.min(l,d))}}}),Mr(t)==null&&e.value&&(r(Ma(e)?Pa(e.value):e.value),V!==null&&a.add(V)),_o(()=>{var n=t();if(e===document.activeElement){var o=V;if(a.has(o))return}Ma(e)&&n===Pa(e.value)||e.type==="date"&&!n&&!e.value||n!==e.value&&(e.value=n??"")})}function Ma(e){var t=e.type;return t==="number"||t==="range"}function Pa(e){return e===""?null:+e}function Vi(e=!1){const t=Ae,r=t.l.u;if(!r)return;let a=()=>er(t.s);if(e){let n=0,o={};const i=ta(()=>{let l=!1;const c=t.s;for(const d in c)c[d]!==o[d]&&(o[d]=c[d],l=!0);return l&&n++,n});a=()=>s(i)}r.b.length&&oi(()=>{Mn(t,a),Ia(r.b)}),Be(()=>{const n=Mr(()=>r.m.map(fs));return()=>{for(const o of n)typeof o=="function"&&o()}}),r.a.length&&Be(()=>{Mn(t,a),Ia(r.a)})}function Mn(e,t){if(e.l.s)for(const r of e.l.s)s(r);t()}const Ui={get(e,t){if(!e.exclude.includes(t))return s(e.version),t in e.special?e.special[t]():e.props[t]},set(e,t,r){if(!(t in e.special)){var a=J;try{dt(e.parent_effect),e.special[t]=Ht({get[t](){return e.props[t]}},t,zn)}finally{dt(a)}}return e.special[t](r),_n(e.version),!0},getOwnPropertyDescriptor(e,t){if(!e.exclude.includes(t)&&t in e.props)return{enumerable:!0,configurable:!0,value:e.props[t]}},deleteProperty(e,t){return e.exclude.includes(t)||(e.exclude.push(t),_n(e.version)),!0},has(e,t){return e.exclude.includes(t)?!1:t in e.props},ownKeys(e){return Reflect.ownKeys(e.props).filter(t=>!e.exclude.includes(t))}};function me(e,t){return new Proxy({props:e,exclude:t,special:{},version:Jt(0),parent_effect:J},Ui)}const Wi={get(e,t){let r=e.props.length;for(;r--;){let a=e.props[r];if(Cr(a)&&(a=a()),typeof a=="object"&&a!==null&&t in a)return a[t]}},set(e,t,r){let a=e.props.length;for(;a--;){let n=e.props[a];Cr(n)&&(n=n());const o=qt(n,t);if(o&&o.set)return o.set(r),!0}return!1},getOwnPropertyDescriptor(e,t){let r=e.props.length;for(;r--;){let a=e.props[r];if(Cr(a)&&(a=a()),typeof a=="object"&&a!==null&&t in a){const n=qt(a,t);return n&&!n.configurable&&(n.configurable=!0),n}}},has(e,t){if(t===Ct||t===Un)return!1;for(let r of e.props)if(Cr(r)&&(r=r()),r!=null&&t in r)return!0;return!1},ownKeys(e){const t=[];for(let r of e.props)if(Cr(r)&&(r=r()),!!r){for(const a in r)t.includes(a)||t.push(a);for(const a of Object.getOwnPropertySymbols(r))t.includes(a)||t.push(a)}return t}};function Ne(...e){return new Proxy({props:e},Wi)}function Ht(e,t,r,a){var N;var n=!Qr||(r&es)!==0,o=(r&ts)!==0,i=(r&rs)!==0,l=a,c=!0,d=()=>(c&&(c=!1,l=i?Mr(a):a),l);let v;if(o){var m=Ct in e||Un in e;v=((N=qt(e,t))==null?void 0:N.set)??(m&&t in e?k=>e[t]=k:void 0)}var _,g=!1;o?[_,g]=Cs(()=>e[t]):_=e[t],_===void 0&&a!==void 0&&(_=d(),v&&(n&&ys(),v(_)));var h;if(n?h=()=>{var k=e[t];return k===void 0?d():(c=!0,k)}:h=()=>{var k=e[t];return k!==void 0&&(l=void 0),k===void 0?l:k},n&&(r&zn)===0)return h;if(v){var $=e.$$legacy;return(function(k,M){return arguments.length>0?((!n||!M||$||g)&&v(M?h():k),k):h()})}var x=!1,f=((r&Qo)!==0?ta:rn)(()=>(x=!1,h()));o&&s(f);var S=J;return(function(k,M){if(arguments.length>0){const C=M?s(f):n&&o?ve(k):k;return T(f,C),x=!0,l!==void 0&&(l=C),k}return Dt&&x||(S.f&Qe)!==0?f.v:s(f)})}const qi="/aonx/v1";function Do(){return localStorage.getItem("kagami_session_token")||""}async function $a(e,t,r=null,a={}){const n=a.token||Do(),o={"Content-Type":"application/json"};n&&(o.Authorization=`Bearer ${n}`);const i=await fetch(qi+t,{method:e,headers:o,body:r?JSON.stringify(r):null});if(a.raw)return i;let l=null;const c=await i.text();if(c)try{l=JSON.parse(c)}catch{l=c}return{status:i.status,data:l}}const Ke=(e,t)=>$a("GET",e,null,t),hr=(e,t,r)=>$a("POST",e,t,r),Pn=(e,t,r)=>$a("PATCH",e,t,r);async function Ki(){const e=await fetch("/metrics");if(!e.ok)return new Map;const t=await e.text(),r=new Map;for(const a of t.split(`
`)){if(!a||a.startsWith("#"))continue;const n=a.match(/^([a-zA-Z_:][a-zA-Z0-9_:]*(?:\{[^}]*\})?)\s+([\d.eE+-]+(?:nan|inf)?)/i);n&&r.set(n[1],parseFloat(n[2]))}return r}const ae=ve({authToken:localStorage.getItem("kagami_auth_token")||"",sessionToken:localStorage.getItem("kagami_session_token")||"",username:localStorage.getItem("kagami_username")||"",acl:localStorage.getItem("kagami_acl")||"",loggedIn:!!localStorage.getItem("kagami_session_token")});function ha(){localStorage.setItem("kagami_auth_token",ae.authToken),localStorage.setItem("kagami_session_token",ae.sessionToken),localStorage.setItem("kagami_username",ae.username),localStorage.setItem("kagami_acl",ae.acl)}async function Gi(e,t){var o,i;const r=await hr("/auth/login",{username:e,password:t});if(r.status!==201)return{ok:!1,error:((o=r.data)==null?void 0:o.reason)||"Login failed"};const a=r.data.token,n=await hr("/session",{client_name:"kagami-admin",client_version:"1.0",hdid:"admin-dashboard-"+crypto.randomUUID().slice(0,8),auth:a});return n.status!==201?{ok:!1,error:((i=n.data)==null?void 0:i.reason)||"Session creation failed"}:(ae.authToken=a,ae.sessionToken=n.data.token,ae.username=r.data.username,ae.acl=r.data.acl,ae.loggedIn=!0,ha(),{ok:!0})}async function Yi(){ae.authToken&&await hr("/auth/logout",null,{token:ae.authToken}),ae.authToken="",ae.sessionToken="",ae.username="",ae.acl="",ae.loggedIn=!1,ha()}async function Ji(){if(!(!ae.sessionToken||(await $a("PATCH","/session",null)).status===200)){if(ae.authToken){const t=await hr("/session",{client_name:"kagami-admin",client_version:"1.0",hdid:"admin-dashboard-"+crypto.randomUUID().slice(0,8),auth:ae.authToken});if(t.status===201){ae.sessionToken=t.data.token,ha();return}}ae.authToken="",ae.sessionToken="",ae.username="",ae.acl="",ae.loggedIn=!1,ha()}}let Hr=null;function Zi(){Hr||(Hr=setInterval(Ji,6e4))}function In(){Hr&&(clearInterval(Hr),Hr=null)}Ps();/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 * 
 * Copyright (c) 2026 Lucide Icons and Contributors
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * ---
 * 
 * The following Lucide icons are derived from the Feather project:
 * 
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 * 
 * The MIT License (MIT) (for the icons listed above)
 * 
 * Copyright (c) 2013-present Cole Bemis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */const Xi={xmlns:"http://www.w3.org/2000/svg",width:24,height:24,viewBox:"0 0 24 24",fill:"none",stroke:"currentColor","stroke-width":2,"stroke-linecap":"round","stroke-linejoin":"round"};/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 * 
 * Copyright (c) 2026 Lucide Icons and Contributors
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * ---
 * 
 * The following Lucide icons are derived from the Feather project:
 * 
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 * 
 * The MIT License (MIT) (for the icons listed above)
 * 
 * Copyright (c) 2013-present Cole Bemis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */const Qi=e=>{for(const t in e)if(t.startsWith("aria-")||t==="role"||t==="title")return!0;return!1};/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 * 
 * Copyright (c) 2026 Lucide Icons and Contributors
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * ---
 * 
 * The following Lucide icons are derived from the Feather project:
 * 
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 * 
 * The MIT License (MIT) (for the icons listed above)
 * 
 * Copyright (c) 2013-present Cole Bemis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */const On=(...e)=>e.filter((t,r,a)=>!!t&&t.trim()!==""&&a.indexOf(t)===r).join(" ").trim();var el=hi("<svg><!><!></svg>");function Te(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]),a=me(r,["name","color","size","strokeWidth","absoluteStrokeWidth","iconNode"]);He(t,!1);let n=Ht(t,"name",8,void 0),o=Ht(t,"color",8,"currentColor"),i=Ht(t,"size",8,24),l=Ht(t,"strokeWidth",8,2),c=Ht(t,"absoluteStrokeWidth",8,!1),d=Ht(t,"iconNode",24,()=>[]);Vi();var v=el();Nn(v,(g,h,$)=>({...Xi,...g,...a,width:i(),height:i(),stroke:o(),"stroke-width":h,class:$}),[()=>Qi(a)?void 0:{"aria-hidden":"true"},()=>(er(c()),er(l()),er(i()),Mr(()=>c()?Number(l())*24/Number(i()):l())),()=>(er(On),er(n()),er(r),Mr(()=>On("lucide-icon","lucide",n()?`lucide-${n()}`:"",r.class)))]);var m=u(v);Ee(m,1,d,Se,(g,h)=>{var $=de(()=>Hn(s(h),2));let x=()=>s($)[0],f=()=>s($)[1];var S=ce(),N=te(S);Mi(N,x,!0,(k,M)=>{Nn(k,()=>({...f()}))}),y(g,S)});var _=b(m);ke(_,t,"default",{}),y(e,v),Ve()}function tl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["circle",{cx:"12",cy:"12",r:"10"}],["path",{d:"M4.929 4.929 19.07 19.071"}]];Te(e,Ne({name:"ban"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function rl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M15 3h6v6"}],["path",{d:"M10 14 21 3"}],["path",{d:"M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"}]];Te(e,Ne({name:"external-link"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function al(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M12 3q1 4 4 6.5t3 5.5a1 1 0 0 1-14 0 5 5 0 0 1 1-3 1 1 0 0 0 5 0c0-2-1.5-3-1.5-5q0-2 2.5-4"}]];Te(e,Ne({name:"flame"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function nl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"m6 14 1.5-2.9A2 2 0 0 1 9.24 10H20a2 2 0 0 1 1.94 2.5l-1.54 6a2 2 0 0 1-1.95 1.5H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H18a2 2 0 0 1 2 2v2"}]];Te(e,Ne({name:"folder-open"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function zo(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M2.586 17.414A2 2 0 0 0 2 18.828V21a1 1 0 0 0 1 1h3a1 1 0 0 0 1-1v-1a1 1 0 0 1 1-1h1a1 1 0 0 0 1-1v-1a1 1 0 0 1 1-1h.172a2 2 0 0 0 1.414-.586l.814-.814a6.5 6.5 0 1 0-4-4z"}],["circle",{cx:"16.5",cy:"7.5",r:".5",fill:"currentColor"}]];Te(e,Ne({name:"key-round"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function ol(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["rect",{width:"7",height:"9",x:"3",y:"3",rx:"1"}],["rect",{width:"7",height:"5",x:"14",y:"3",rx:"1"}],["rect",{width:"7",height:"9",x:"14",y:"12",rx:"1"}],["rect",{width:"7",height:"5",x:"3",y:"16",rx:"1"}]];Te(e,Ne({name:"layout-dashboard"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function sl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"m16 17 5-5-5-5"}],["path",{d:"M21 12H9"}],["path",{d:"M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"}]];Te(e,Ne({name:"log-out"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function il(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M14.106 5.553a2 2 0 0 0 1.788 0l3.659-1.83A1 1 0 0 1 21 4.619v12.764a1 1 0 0 1-.553.894l-4.553 2.277a2 2 0 0 1-1.788 0l-4.212-2.106a2 2 0 0 0-1.788 0l-3.659 1.83A1 1 0 0 1 3 19.381V6.618a1 1 0 0 1 .553-.894l4.553-2.277a2 2 0 0 1 1.788 0z"}],["path",{d:"M15 5.764v15"}],["path",{d:"M9 3.236v15"}]];Te(e,Ne({name:"map"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function ll(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M4 5h16"}],["path",{d:"M4 12h16"}],["path",{d:"M4 19h16"}]];Te(e,Ne({name:"menu"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function cl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M22 17a2 2 0 0 1-2 2H6.828a2 2 0 0 0-1.414.586l-2.202 2.202A.71.71 0 0 1 2 21.286V5a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2z"}]];Te(e,Ne({name:"message-square"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function dl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M20.985 12.486a9 9 0 1 1-9.473-9.472c.405-.022.617.46.402.803a6 6 0 0 0 8.268 8.268c.344-.215.825-.004.803.401"}]];Te(e,Ne({name:"moon"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function ul(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"}],["path",{d:"M21 3v5h-5"}],["path",{d:"M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"}],["path",{d:"M8 16H3v5"}]];Te(e,Ne({name:"refresh-cw"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function fl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z"}],["path",{d:"M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7"}],["path",{d:"M7 3v4a1 1 0 0 0 1 1h7"}]];Te(e,Ne({name:"save"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function vl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M9.671 4.136a2.34 2.34 0 0 1 4.659 0 2.34 2.34 0 0 0 3.319 1.915 2.34 2.34 0 0 1 2.33 4.033 2.34 2.34 0 0 0 0 3.831 2.34 2.34 0 0 1-2.33 4.033 2.34 2.34 0 0 0-3.319 1.915 2.34 2.34 0 0 1-4.659 0 2.34 2.34 0 0 0-3.32-1.915 2.34 2.34 0 0 1-2.33-4.033 2.34 2.34 0 0 0 0-3.831A2.34 2.34 0 0 1 6.35 6.051a2.34 2.34 0 0 0 3.319-1.915"}],["circle",{cx:"12",cy:"12",r:"3"}]];Te(e,Ne({name:"settings"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function pl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"}]];Te(e,Ne({name:"shield"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function hl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["circle",{cx:"12",cy:"12",r:"4"}],["path",{d:"M12 2v2"}],["path",{d:"M12 20v2"}],["path",{d:"m4.93 4.93 1.41 1.41"}],["path",{d:"m17.66 17.66 1.41 1.41"}],["path",{d:"M2 12h2"}],["path",{d:"M20 12h2"}],["path",{d:"m6.34 17.66-1.41 1.41"}],["path",{d:"m19.07 4.93-1.41 1.41"}]];Te(e,Ne({name:"sun"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function _l(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"}],["path",{d:"M16 3.128a4 4 0 0 1 0 7.744"}],["path",{d:"M22 21v-2a4 4 0 0 0-3-3.87"}],["circle",{cx:"9",cy:"7",r:"4"}]];Te(e,Ne({name:"users"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}function xl(e,t){const r=me(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M18 6 6 18"}],["path",{d:"m6 6 12 12"}]];Te(e,Ne({name:"x"},()=>r,{get iconNode(){return a},children:(n,o)=>{var i=ce(),l=te(i);ke(l,t,"default",{}),y(n,i)},$$slots:{default:!0}}))}const bl=(e,t=cr)=>{var r=ce(),a=te(r);Ti(a,t,(n,o)=>{o(n,{size:15,strokeWidth:1.5})}),y(e,r)};var ml=E("<button><!> </button>"),gl=E('<div class="fixed inset-0 bg-black/40 z-30 md:hidden"></div>'),yl=E(`<button class="md:hidden fixed top-3 left-3 z-50 p-2 bg-(--color-surface-2) border border-(--color-border)" aria-label="Toggle navigation"><!></button> <aside><div class="px-4 py-3 border-b border-(--color-border)"><h1 class="text-sm font-semibold tracking-wide uppercase text-(--color-accent)">Kagami</h1> <p class="text-xs text-(--color-text-muted) mt-0.5"> </p></div> <nav class="flex-1 overflow-y-auto py-1"><!> <a href="/grafana/" target="_blank" rel="noopener" class="w-full flex items-center gap-2.5 px-4 py-1.5 text-sm text-(--color-text-secondary)
             hover:bg-(--color-surface-2) hover:text-(--color-text-primary)"><!> Monitoring</a></nav> <div class="px-3 py-2 border-t border-(--color-border) flex items-center justify-between"><button class="flex items-center gap-1.5 px-2 py-1 text-xs text-(--color-text-muted) hover:text-red-500 transition-colors"><!> Logout</button> <button class="p-1 text-(--color-text-muted) hover:text-(--color-text-primary) transition-colors" aria-label="Toggle theme"><!></button></div></aside> <!>`,1);function wl(e,t){He(t,!0);let r=Ht(t,"currentPage",3,"dashboard"),a=F(!1),n=F(!document.documentElement.classList.contains("light"));function o(){T(n,!s(n)),document.documentElement.classList.toggle("light",!s(n)),localStorage.setItem("kagami_theme",s(n)?"dark":"light")}Be(()=>{localStorage.getItem("kagami_theme")==="light"&&(T(n,!1),document.documentElement.classList.add("light"))});const i=[{page:"dashboard",label:"Dashboard",icon:ol},{page:"sessions",label:"Players",icon:_l},{page:"traffic",label:"Traffic",icon:cl},{page:"config",label:"Config",icon:vl},{page:"bans",label:"Bans",icon:tl},{page:"moderation",label:"Moderation",icon:pl},{page:"areas",label:"Areas",icon:il},{page:"users",label:"Accounts",icon:zo},{page:"firewall",label:"Firewall",icon:al},{page:"content",label:"Content",icon:nl}];function l(w){window.location.hash="#/"+w,T(a,!1)}async function c(){await Yi(),window.location.hash="#/login"}var d=yl(),v=te(d),m=u(v);{var _=w=>{xl(w,{size:18})},g=w=>{ll(w,{size:18})};q(m,w=>{s(a)?w(_):w(g,-1)})}var h=b(v,2),$=u(h),x=b(u($),2),f=u(x),S=b($,2),N=u(S);Ee(N,17,()=>i,Se,(w,L)=>{let W=()=>s(L).page,A=()=>s(L).label,R=()=>s(L).icon;var Z=ml(),ne=u(Z);bl(ne,R);var ue=b(ne);j(()=>{xe(Z,1,`w-full flex items-center gap-2.5 px-4 py-1.5 text-sm transition-colors
               ${r()===W()?"bg-(--color-surface-2) text-(--color-accent) font-medium":"text-(--color-text-secondary) hover:bg-(--color-surface-2) hover:text-(--color-text-primary)"}`),P(ue,` ${A()??""}`)}),re("click",Z,()=>l(W())),y(w,Z)});var k=b(N,2),M=u(k);rl(M,{size:15,strokeWidth:1.5});var C=b(S,2),I=u(C),D=u(I);sl(D,{size:13,strokeWidth:1.5});var O=b(I,2),H=u(O);{var z=w=>{hl(w,{size:14,strokeWidth:1.5})},U=w=>{dl(w,{size:14,strokeWidth:1.5})};q(H,w=>{s(n)?w(z):w(U,-1)})}var G=b(h,2);{var B=w=>{var L=gl();re("click",L,()=>T(a,!1)),y(w,L)};q(G,w=>{s(a)&&w(B)})}j(()=>{xe(h,1,`fixed md:static inset-y-0 left-0 z-40 w-52 bg-(--color-surface-1) border-r border-(--color-border)
         flex flex-col transition-transform duration-150
         ${s(a)?"translate-x-0":"-translate-x-full"} md:translate-x-0`),P(f,`${ae.username??""} · ${ae.acl??""}`)}),re("click",v,()=>T(a,!s(a))),re("click",I,c),re("click",O,o),y(e,d),Ve()}zt(["click"]);var $l=E('<div class="text-xs text-red-500 bg-red-500/10 border border-red-500/20 px-3 py-2"> </div>'),kl=E(`<div class="min-h-screen bg-(--color-surface-0) flex items-center justify-center px-4"><div class="w-full max-w-xs"><div class="bg-(--color-surface-1) border border-(--color-border) p-6"><div class="flex items-center gap-2 mb-1"><!> <h1 class="text-lg font-semibold tracking-wide uppercase">Kagami</h1></div> <p class="text-xs text-(--color-text-muted) mb-6">Server Administration</p> <form class="space-y-4"><div><label for="username" class="block text-xs font-medium text-(--color-text-secondary) mb-1">Username</label> <input id="username" type="text" required="" autocomplete="username" class="w-full px-3 py-2 bg-(--color-surface-2) border border-(--color-border) text-sm text-(--color-text-primary)
                   placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)" placeholder="root"/></div> <div><label for="password" class="block text-xs font-medium text-(--color-text-secondary) mb-1">Password</label> <input id="password" type="password" required="" autocomplete="current-password" class="w-full px-3 py-2 bg-(--color-surface-2) border border-(--color-border) text-sm text-(--color-text-primary)
                   placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)"/></div> <!> <button type="submit" class="w-full py-2 px-4 bg-(--color-accent) text-(--color-surface-0) text-sm font-medium
                 hover:opacity-80 disabled:opacity-30 transition-opacity"> </button></form></div></div></div>`);function Sl(e,t){He(t,!0);let r=F(""),a=F(""),n=F(""),o=F(!1);async function i(M){M.preventDefault(),T(n,""),T(o,!0);const C=await Gi(s(r),s(a));T(o,!1),C.ok||T(n,C.error,!0)}var l=kl(),c=u(l),d=u(c),v=u(d),m=u(v);zo(m,{size:18,strokeWidth:1.5});var _=b(v,4),g=u(_),h=b(u(g),2),$=b(g,2),x=b(u($),2),f=b($,2);{var S=M=>{var C=$l(),I=u(C);j(()=>P(I,s(n))),y(M,C)};q(f,M=>{s(n)&&M(S)})}var N=b(f,2),k=u(N);j(()=>{N.disabled=s(o),P(k,s(o)?"Signing in...":"Sign in")}),Po("submit",_,i),Pr(h,()=>s(r),M=>T(r,M)),Pr(x,()=>s(a),M=>T(a,M)),y(e,l),Ve()}const xr=(e,t=cr,r=cr,a=cr)=>{var n=El(),o=u(n),i=u(o),l=b(i),c=u(l),d=b(o,2),v=u(d);j(()=>{P(i,r()),P(c,a()),P(v,t())}),y(e,n)};var El=E('<div class="bg-(--color-surface-1) p-3"><div class="text-xl font-semibold tabular-nums"> <span class="text-xs text-(--color-text-muted) font-normal"> </span></div> <div class="text-[10px] uppercase tracking-wider text-(--color-text-muted) mt-0.5"> </div></div>'),Al=E('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Nl=E('<span class="text-xs text-(--color-text-secondary)"> </span>'),Tl=E('<div class="bg-(--color-surface-1) border border-(--color-border) px-4 py-3 flex flex-wrap items-baseline gap-x-3 gap-y-1"><span class="font-semibold text-sm"> </span> <span class="text-xs text-(--color-text-muted)"> </span> <!></div>'),Ml=E('<span class="text-[10px] px-1 py-px bg-cyan-500/15 text-cyan-400 font-medium"> </span>'),Pl=E('<span class="text-[10px] px-1 py-px bg-amber-500/15 text-amber-400 font-medium"> </span>'),Il=E('<div class="px-4 py-1.5 flex items-center justify-between text-sm"><div class="flex items-center gap-2"><span> </span> <!> <!></div> <span class="text-xs text-(--color-text-muted) tabular-nums"> </span></div>'),Ol=E('<a href="#/sessions" class="text-xs text-(--color-text-muted) hover:text-(--color-text-primary)">View all</a>'),Cl=E('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1"> </td><td class="px-4 py-1 text-(--color-text-secondary)"> </td><td class="px-4 py-1 text-right text-(--color-text-muted) tabular-nums"> </td></tr>'),Ll=E('<tr><td colspan="3" class="px-4 py-4 text-center text-(--color-text-muted) text-xs">No active sessions</td></tr>'),Rl=E('<!> <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-px bg-(--color-border)"><!> <!> <!> <!> <!> <!></div> <div class="grid lg:grid-cols-2 gap-5"><div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="px-4 py-2 border-b border-(--color-border)"><h3 class="text-xs font-semibold uppercase tracking-wide text-(--color-text-muted)">Areas</h3></div> <div class="divide-y divide-(--color-border)"></div></div> <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="px-4 py-2 border-b border-(--color-border) flex items-center justify-between"><h3 class="text-xs font-semibold uppercase tracking-wide text-(--color-text-muted)">Active Sessions</h3> <!></div> <div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-1.5">Name</th><th class="px-4 py-1.5">Area</th><th class="px-4 py-1.5 text-right">Idle</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div></div>',1),Dl=E('<div class="space-y-5"><h2 class="text-lg font-semibold">Dashboard</h2> <!></div>');function zl(e,t){He(t,!0);let r=F(null),a=F(ve([])),n=F(ve([])),o=F(ve(new Map)),i=F(!0);async function l(){var f;const[g,h,$,x]=await Promise.all([Ke("/server"),Ke("/admin/sessions"),Ke("/areas"),Ki()]);g.status===200&&T(r,g.data,!0),h.status===200&&T(a,h.data,!0),$.status===200&&T(n,((f=$.data)==null?void 0:f.areas)||[],!0),T(o,x,!0),T(i,!1)}Be(()=>{l();const g=setInterval(l,1e4);return()=>clearInterval(g)});function c(g){return s(o).get(g)??0}var d=Dl(),v=b(u(d),2);{var m=g=>{var h=Al();y(g,h)},_=g=>{var h=Rl(),$=te(h);{var x=A=>{var R=Tl(),Z=u(R),ne=u(Z),ue=b(Z,2),X=u(ue),Y=b(ue,2);{var se=ie=>{var oe=Nl(),Me=u(oe);j(()=>P(Me,s(r).description)),y(ie,oe)};q(Y,ie=>{s(r).description&&ie(se)})}j(()=>{P(ne,s(r).name),P(X,`v${s(r).version??""}`)}),y(A,R)};q($,A=>{s(r)&&A(x)})}var f=b($,2),S=u(f);xr(S,()=>"Players",()=>{var A;return((A=s(r))==null?void 0:A.online)??0},()=>{var A;return"/"+(((A=s(r))==null?void 0:A.max)??"?")});var N=b(S,2);{let A=de(()=>c("kagami_ws_connections"));xr(N,()=>"WS Clients",()=>s(A),()=>"")}var k=b(N,2);xr(k,()=>"REST",()=>s(a).length,()=>"");var M=b(k,2);xr(M,()=>"Areas",()=>s(n).length,()=>"");var C=b(M,2);{let A=de(()=>c("kagami_sessions_moderators"));xr(C,()=>"Mods",()=>s(A),()=>"")}var I=b(C,2);{let A=de(()=>c("kagami_characters_taken"));xr(I,()=>"Chars",()=>s(A),()=>"")}var D=b(f,2),O=u(D),H=b(u(O),2);Ee(H,21,()=>s(n),Se,(A,R)=>{var Z=Il(),ne=u(Z),ue=u(ne),X=u(ue),Y=b(ue,2);{var se=ge=>{var Re=Ml(),he=u(Re);j(()=>P(he,s(R).status)),y(ge,Re)};q(Y,ge=>{s(R).status&&s(R).status!=="IDLE"&&ge(se)})}var ie=b(Y,2);{var oe=ge=>{var Re=Pl(),he=u(Re);j(()=>P(he,s(R).locked)),y(ge,Re)};q(ie,ge=>{s(R).locked&&s(R).locked!=="FREE"&&ge(oe)})}var Me=b(ne,2),ut=u(Me);j(()=>{P(X,s(R).name),P(ut,s(R).players??0)}),y(A,Z)});var z=b(O,2),U=u(z),G=b(u(U),2);{var B=A=>{var R=Ol();y(A,R)};q(G,A=>{s(a).length>15&&A(B)})}var w=b(U,2),L=u(w),W=b(u(L));Ee(W,21,()=>s(a).slice(0,15),Se,(A,R)=>{var Z=Cl(),ne=u(Z),ue=u(ne),X=b(ne),Y=u(X),se=b(X),ie=u(se);j(()=>{P(ue,s(R).display_name||"(anon)"),P(Y,s(R).area),P(ie,`${s(R).idle_seconds??""}s`)}),y(A,Z)},A=>{var R=Ll();y(A,R)}),y(g,h)};q(v,g=>{s(i)?g(m):g(_,-1)})}y(e,d),Ve()}var jl=E('<div class="text-xs text-red-500 bg-red-500/10 border border-red-500/20 px-3 py-2"> </div>'),Fl=E('<pre class="text-xs text-emerald-400 bg-emerald-400/10 border border-emerald-400/20 px-3 py-2 whitespace-pre-wrap"> </pre>'),Bl=E('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Hl=E('<span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>'),Vl=E('<button><span class="truncate"> </span> <!></button>'),Ul=E('<span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>'),Wl=E('<span class="text-[10px] text-(--color-text-muted) font-mono truncate ml-2 max-w-16"> </span>'),ql=E('<button><span class="truncate"> </span> <!></button>'),Kl=E('<div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)"></div>'),Gl=E('<p class="text-(--color-text-muted) text-sm">Select a key to edit</p>'),Yl=E('<input type="checkbox" class="accent-(--color-accent)"/>'),Jl=E('<input type="number" step="any" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"/>'),Zl=E('<textarea rows="2" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"></textarea>'),Xl=E('<input type="text" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)"/>'),Ql=E('<div class="flex items-center gap-3"><span class="text-xs text-(--color-text-secondary) w-44 shrink-0 truncate"> </span> <!></div>'),ec=E('<h3 class="text-xs font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3"> </h3> <div class="space-y-2"></div>',1),tc=E('<input type="checkbox" class="accent-(--color-accent)"/> <span class="text-sm"> </span>',1),rc=E('<input type="number" step="any" class="w-48 px-2 py-1 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"/>'),ac=E('<input type="text" class="flex-1 px-2 py-1 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)"/>'),nc=E('<h3 class="text-xs font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3"> </h3> <div class="flex items-center gap-3"><!></div>',1),oc=E('<div class="flex border border-(--color-border) overflow-hidden" style="height: 65vh"><div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)"></div> <!> <div class="flex-1 overflow-y-auto bg-(--color-surface-0) p-4"><!></div></div>'),sc=E('<div class="space-y-4"><div class="flex items-center justify-between flex-wrap gap-2"><h2 class="text-lg font-semibold">Configuration</h2> <div class="flex gap-px"><button class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-surface-3) border border-(--color-border) text-(--color-text-secondary) hover:text-(--color-text-primary) disabled:opacity-30"><!> Reload</button> <button class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-accent) text-(--color-surface-0) hover:opacity-80 disabled:opacity-30"><!> </button></div></div> <!> <!> <!></div>');function ic(e,t){He(t,!0);let r=F(null),a=F(null),n=F(!0),o=F(!1),i=F(""),l=F(""),c=F(ve([]));async function d(){var L;T(n,!0);const w=await Ke("/admin/config");w.status===200?(T(r,w.data,!0),T(a,JSON.parse(JSON.stringify(w.data)),!0),T(c,[],!0)):T(l,((L=w.data)==null?void 0:L.reason)||"Failed to load config",!0),T(n,!1)}Be(()=>{d()});function v(w){let L=s(r);for(let W=0;W<w;W++){if(!L||typeof L!="object")return null;L=L[s(c)[W]]}return L}function m(w,L){T(c,[...s(c).slice(0,w),L],!0)}function _(w){return w!==null&&typeof w=="object"&&!Array.isArray(w)}function g(w,L){const W={};for(const A of Object.keys(L))if(_(L[A])&&_(w==null?void 0:w[A])){const R=g(w[A],L[A]);Object.keys(R).length>0&&(W[A]=R)}else JSON.stringify(L[A])!==JSON.stringify(w==null?void 0:w[A])&&(W[A]=L[A]);return W}async function h(){var W,A;T(l,""),T(i,"");const w=g(s(a),s(r));if(Object.keys(w).length===0){T(i,"No changes.");return}T(o,!0);const L=await Pn("/admin/config",w);T(o,!1),L.status===200?(T(i,((W=L.data)==null?void 0:W.reload_summary)||"Saved.",!0),T(a,JSON.parse(JSON.stringify(s(r))),!0)):T(l,((A=L.data)==null?void 0:A.reason)||"Failed",!0)}async function $(){var L,W;T(i,""),T(o,!0);const w=await Pn("/admin/config",{});T(o,!1),w.status===200?T(i,((L=w.data)==null?void 0:L.reload_summary)||"Reloaded.",!0):T(l,((W=w.data)==null?void 0:W.reason)||"Failed",!0)}var x=sc(),f=u(x),S=b(u(f),2),N=u(S),k=u(N);ul(k,{size:13,strokeWidth:1.5});var M=b(N,2),C=u(M);fl(C,{size:13,strokeWidth:1.5});var I=b(C),D=b(f,2);{var O=w=>{var L=jl(),W=u(L);j(()=>P(W,s(l))),y(w,L)};q(D,w=>{s(l)&&w(O)})}var H=b(D,2);{var z=w=>{var L=Fl(),W=u(L);j(()=>P(W,s(i))),y(w,L)};q(H,w=>{s(i)&&w(z)})}var U=b(H,2);{var G=w=>{var L=Bl();y(w,L)},B=w=>{var L=oc(),W=u(L);Ee(W,21,()=>Object.keys(s(r)),Se,(X,Y)=>{const se=de(()=>s(r)[s(Y)]);var ie=Vl(),oe=u(ie),Me=u(oe),ut=b(oe,2);{var ge=he=>{var ye=Hl();y(he,ye)},Re=de(()=>_(s(se)));q(ut,he=>{s(Re)&&he(ge)})}j(()=>{xe(ie,1,`w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                   ${s(c)[0]===s(Y)?"bg-(--color-surface-2) text-(--color-accent) font-medium":"text-(--color-text-secondary)"}`),P(Me,s(Y))}),re("click",ie,()=>m(0,s(Y))),y(X,ie)});var A=b(W,2);Ee(A,17,()=>s(c),Se,(X,Y,se)=>{const ie=de(()=>v(se)),oe=de(()=>{var he;return(he=s(ie))==null?void 0:he[s(Y)]});var Me=ce(),ut=te(Me);{var ge=he=>{var ye=Kl();Ee(ye,21,()=>Object.keys(s(oe)),Se,(ft,Ge)=>{const mt=de(()=>s(oe)[s(Ge)]);var gt=ql(),yt=u(gt),Zt=u(yt),jt=b(yt,2);{var et=Ce=>{var Ft=Ul();y(Ce,Ft)},fe=de(()=>_(s(mt))),De=Ce=>{var Ft=Wl(),Or=u(Ft);j(Xt=>P(Or,Xt),[()=>typeof s(mt)=="string"?s(mt):JSON.stringify(s(mt))]),y(Ce,Ft)};q(jt,Ce=>{s(fe)?Ce(et):Ce(De,-1)})}j(()=>{xe(gt,1,`w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                       ${s(c)[se+1]===s(Ge)?"bg-(--color-surface-2) text-(--color-accent) font-medium":"text-(--color-text-secondary)"}`),P(Zt,s(Ge))}),re("click",gt,()=>m(se+1,s(Ge))),y(ft,gt)}),y(he,ye)},Re=de(()=>_(s(oe)));q(ut,he=>{s(Re)&&he(ge)})}y(X,Me)});var R=b(A,2),Z=u(R);{var ne=X=>{var Y=Gl();y(X,Y)},ue=X=>{const Y=de(()=>s(c).length-1),se=de(()=>v(s(Y))),ie=de(()=>s(c)[s(Y)]),oe=de(()=>{var ye;return(ye=s(se))==null?void 0:ye[s(ie)]});var Me=ce(),ut=te(Me);{var ge=ye=>{var ft=ec(),Ge=te(ft),mt=u(Ge),gt=b(Ge,2);Ee(gt,21,()=>Object.entries(s(oe)),Se,(yt,Zt)=>{var jt=de(()=>Hn(s(Zt),2));let et=()=>s(jt)[0],fe=()=>s(jt)[1];var De=ce(),Ce=te(De);{var Ft=Xt=>{var dn=Ql(),ka=u(dn),jo=u(ka),Fo=b(ka,2);{var Bo=tt=>{var ze=Yl();j(()=>An(ze,fe())),re("change",ze,Nt=>s(oe)[et()]=Nt.target.checked),y(tt,ze)},Ho=tt=>{var ze=Jl();j(()=>zr(ze,fe())),re("change",ze,Nt=>s(oe)[et()]=Number(Nt.target.value)),y(tt,ze)},Vo=tt=>{var ze=Zl();j(Nt=>zr(ze,Nt),[()=>JSON.stringify(fe(),null,2)]),re("change",ze,Nt=>{try{s(oe)[et()]=JSON.parse(Nt.target.value)}catch{}}),y(tt,ze)},Uo=de(()=>Array.isArray(fe())),Wo=tt=>{var ze=Xl();j(()=>zr(ze,fe()??"")),re("input",ze,Nt=>s(oe)[et()]=Nt.target.value),y(tt,ze)};q(Fo,tt=>{typeof fe()=="boolean"?tt(Bo):typeof fe()=="number"?tt(Ho,1):s(Uo)?tt(Vo,2):tt(Wo,-1)})}j(()=>{Ka(ka,"title",et()),P(jo,et())}),y(Xt,dn)},Or=de(()=>!_(fe()));q(Ce,Xt=>{s(Or)&&Xt(Ft)})}y(yt,De)}),j(yt=>P(mt,yt),[()=>s(c).join(" / ")]),y(ye,ft)},Re=de(()=>_(s(oe))),he=ye=>{var ft=nc(),Ge=te(ft),mt=u(Ge),gt=b(Ge,2),yt=u(gt);{var Zt=fe=>{var De=tc(),Ce=te(De),Ft=b(Ce,2),Or=u(Ft);j(()=>{An(Ce,s(oe)),P(Or,s(oe)?"true":"false")}),re("change",Ce,Xt=>s(se)[s(ie)]=Xt.target.checked),y(fe,De)},jt=fe=>{var De=rc();j(()=>zr(De,s(oe))),re("change",De,Ce=>s(se)[s(ie)]=Number(Ce.target.value)),y(fe,De)},et=fe=>{var De=ac();j(()=>zr(De,s(oe)??"")),re("input",De,Ce=>s(se)[s(ie)]=Ce.target.value),y(fe,De)};q(yt,fe=>{typeof s(oe)=="boolean"?fe(Zt):typeof s(oe)=="number"?fe(jt,1):fe(et,-1)})}j(fe=>P(mt,fe),[()=>s(c).join(" / ")]),y(ye,ft)};q(ut,ye=>{s(Re)?ye(ge):ye(he,-1)})}y(X,Me)};q(Z,X=>{s(c).length===0?X(ne):X(ue,-1)})}y(w,L)};q(U,w=>{s(n)?w(G):s(r)&&w(B,1)})}j(()=>{N.disabled=s(o),M.disabled=s(o),P(I,` ${s(o)?"Saving...":"Save"}`)}),re("click",N,$),re("click",M,h),y(e,x),Ve()}zt(["click","change","input"]);const rt=(e,t=cr,r=cr)=>{var a=lc(),n=u(a),o=u(n),i=b(n,2),l=u(i);j(()=>{P(o,t()),P(l,r())}),y(e,a)};var lc=E('<div><div class="text-(--color-text-muted) text-[10px] uppercase tracking-wider"> </div> <div class="text-(--color-text-primary) font-mono mt-0.5 truncate"> </div></div>'),cc=E('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),dc=E('<tr><td class="px-4 py-1.5 font-medium"> </td><td class="px-4 py-1.5 text-(--color-text-secondary)"> </td><td class="px-4 py-1.5 text-(--color-text-secondary)"> </td><td class="px-4 py-1.5 text-(--color-text-muted) hidden sm:table-cell truncate max-w-32"> </td><td class="px-4 py-1.5 text-right text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td><td class="px-4 py-1.5 text-right text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td><td class="px-4 py-1.5 text-right text-(--color-text-muted) tabular-nums"> </td></tr>'),uc=E('<div class="bg-(--color-surface-1) border border-(--color-border) p-4 space-y-3"><h3 class="text-sm font-semibold"> </h3> <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-4 gap-3 text-xs"><!> <!> <!> <!> <!> <!> <!> <!> <!> <!> <!> <!></div></div>'),fc=E('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">Name</th><th class="px-4 py-2">Protocol</th><th class="px-4 py-2">Area</th><th class="px-4 py-2 hidden sm:table-cell">Client</th><th class="px-4 py-2 text-right hidden md:table-cell">Sent</th><th class="px-4 py-2 text-right hidden md:table-cell">Recv</th><th class="px-4 py-2 text-right">Idle</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div> <!>',1),vc=E('<div class="space-y-4"><h2 class="text-lg font-semibold">Players <span class="text-sm text-(--color-text-muted) font-normal"> </span></h2> <!></div>');function pc(e,t){He(t,!0);let r=F(ve([])),a=F(!0),n=F(null);async function o(){const h=await Ke("/admin/sessions");h.status===200&&T(r,h.data,!0),T(a,!1)}Be(()=>{o();const h=setInterval(o,5e3);return()=>clearInterval(h)});function i(h){return h<1024?h+" B":h<1024*1024?(h/1024).toFixed(1)+" KB":(h/(1024*1024)).toFixed(1)+" MB"}var l=vc(),c=u(l),d=b(u(c)),v=u(d),m=b(c,2);{var _=h=>{var $=cc();y(h,$)},g=h=>{var $=fc(),x=te($),f=u(x),S=u(f),N=b(u(S));Ee(N,21,()=>s(r),Se,(C,I)=>{var D=dc(),O=u(D),H=u(O),z=b(O),U=u(z),G=b(z),B=u(G),w=b(G),L=u(w),W=b(w),A=u(W),R=b(W),Z=u(R),ne=b(R),ue=u(ne);j((X,Y)=>{var se;xe(D,1,`hover:bg-(--color-surface-2)/50 cursor-pointer ${((se=s(n))==null?void 0:se.session_id)===s(I).session_id?"bg-(--color-surface-2)":""}`),P(H,s(I).display_name||"(anon)"),P(U,s(I).protocol),P(B,s(I).area),P(L,s(I).client_software),P(A,X),P(Z,Y),P(ue,`${s(I).idle_seconds??""}s`)},[()=>i(s(I).bytes_sent),()=>i(s(I).bytes_received)]),re("click",D,()=>{var X;return T(n,((X=s(n))==null?void 0:X.session_id)===s(I).session_id?null:s(I),!0)}),y(C,D)});var k=b(x,2);{var M=C=>{var I=uc(),D=u(I),O=u(D),H=b(D,2),z=u(H);rt(z,()=>"Session ID",()=>s(n).session_id);var U=b(z,2);rt(U,()=>"Protocol",()=>s(n).protocol);var G=b(U,2);rt(G,()=>"Area",()=>s(n).area);var B=b(G,2);rt(B,()=>"Character",()=>s(n).character_id>=0?"#"+s(n).character_id:"None");var w=b(B,2);rt(w,()=>"HDID",()=>s(n).hardware_id);var L=b(w,2);rt(L,()=>"Client",()=>s(n).client_software);var W=b(L,2);rt(W,()=>"Packets Sent",()=>s(n).packets_sent);var A=b(W,2);rt(A,()=>"Packets Recv",()=>s(n).packets_received);var R=b(A,2);rt(R,()=>"Mod Actions",()=>s(n).mod_actions);var Z=b(R,2);{let X=de(()=>i(s(n).bytes_sent));rt(Z,()=>"Bytes Sent",()=>s(X))}var ne=b(Z,2);{let X=de(()=>i(s(n).bytes_received));rt(ne,()=>"Bytes Recv",()=>s(X))}var ue=b(ne,2);rt(ue,()=>"Idle",()=>s(n).idle_seconds+"s"),j(()=>P(O,s(n).display_name||"(anonymous)")),y(C,I)};q(k,C=>{s(n)&&C(M)})}y(h,$)};q(m,h=>{s(a)?h(_):h(g,-1)})}j(()=>P(v,`(${s(r).length??""})`)),y(e,l),Ve()}zt(["click"]);var hc=E('<span class="text-(--color-text-muted) shrink-0"> </span>'),_c=E('<div class="px-3 py-1 flex gap-2 hover:bg-(--color-surface-2)/50 border-b border-(--color-border)/30"><span class="text-(--color-text-muted) shrink-0 w-16"> </span> <span> </span> <!> <span class="text-amber-400 shrink-0"> </span> <span class="text-(--color-text-primary) break-all"> </span></div>'),xc=E('<div class="px-4 py-12 text-center text-(--color-text-muted)"> </div>'),bc=E(`<div class="space-y-4"><div class="flex items-center justify-between flex-wrap gap-2"><div class="flex items-center gap-2"><h2 class="text-lg font-semibold">Traffic</h2> <span></span></div> <input type="text" placeholder="Filter..." class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
             placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-56"/></div> <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="max-h-[75vh] overflow-y-auto font-mono text-xs leading-relaxed"></div></div></div>`);function mc(e,t){He(t,!0);let r=F(ve([])),a=F(!1),n=F(""),o=500;function i(){const f=Do();f&&l(f)}async function l(f){try{const S=await fetch("/aonx/v1/events",{headers:{Authorization:`Bearer ${f}`}});if(!S.ok){T(a,!1);return}T(a,!0);const N=S.body.getReader(),k=new TextDecoder;let M="";for(;;){const{done:C,value:I}=await N.read();if(C)break;M+=k.decode(I,{stream:!0});const D=M.split(`
`);M=D.pop()||"";let O="",H="";for(const z of D)z.startsWith("event: ")?O=z.slice(7):z.startsWith("data: ")?H=z.slice(6):z===""&&O&&H&&(c(O,H),O="",H="")}}catch(S){console.error("SSE error:",S)}T(a,!1)}function c(f,S){if(!(f!=="ic_message"&&f!=="ooc_message"))try{const N=JSON.parse(S);T(r,[{type:f==="ic_message"?"IC":"OOC",name:N.showname||N.name||N.character||"???",text:N.message||"",time:new Date().toLocaleTimeString("en-US",{hour12:!1}),area:N.area||""},...s(r).slice(0,o-1)],!0)}catch{}}Be(()=>{i()});let d=de(()=>s(n)?s(r).filter(f=>f.text.toLowerCase().includes(s(n).toLowerCase())||f.name.toLowerCase().includes(s(n).toLowerCase())||f.area.toLowerCase().includes(s(n).toLowerCase())):s(r));var v=bc(),m=u(v),_=u(m),g=b(u(_),2),h=b(_,2),$=b(m,2),x=u($);Ee(x,21,()=>s(d),Se,(f,S)=>{var N=_c(),k=u(N),M=u(k),C=b(k,2),I=u(C),D=b(C,2);{var O=B=>{var w=hc(),L=u(w);j(()=>P(L,`[${s(S).area??""}]`)),y(B,w)};q(D,B=>{s(S).area&&B(O)})}var H=b(D,2),z=u(H),U=b(H,2),G=u(U);j(()=>{P(M,s(S).time),xe(C,1,`shrink-0 w-6 text-center font-bold ${s(S).type==="IC"?"text-cyan-400":"text-emerald-400"}`),P(I,s(S).type),P(z,s(S).name),P(G,s(S).text)}),y(f,N)},f=>{var S=xc(),N=u(S);j(()=>P(N,s(a)?"Waiting for messages...":"Not connected")),y(f,S)}),j(()=>xe(g,1,`w-1.5 h-1.5 ${s(a)?"bg-emerald-500":"bg-red-500"}`)),Pr(h,()=>s(n),f=>T(n,f)),y(e,v),Ve()}var gc=E('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),yc=E('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-mono text-xs"> </td><td class="px-4 py-1.5 max-w-xs truncate"> </td><td class="px-4 py-1.5 text-(--color-text-secondary) hidden md:table-cell"> </td><td class="px-4 py-1.5 text-(--color-text-muted) text-xs hidden lg:table-cell"> </td><td class="px-4 py-1.5"><span> </span></td><td class="px-4 py-1.5"><button class="text-xs text-red-400 hover:text-red-300">Unban</button></td></tr>'),wc=E('<tr><td colspan="6" class="px-4 py-8 text-center text-(--color-text-muted) text-sm">No bans found</td></tr>'),$c=E('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">IPID</th><th class="px-4 py-2">Reason</th><th class="px-4 py-2 hidden md:table-cell">By</th><th class="px-4 py-2 hidden lg:table-cell">When</th><th class="px-4 py-2">Duration</th><th class="px-4 py-2"></th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),kc=E(`<div class="space-y-4"><div class="flex items-center justify-between flex-wrap gap-2"><h2 class="text-lg font-semibold">Bans</h2> <form class="flex gap-px"><input type="text" placeholder="Search IPID, HDID, reason..." class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
               placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-56"/> <button type="submit" class="px-3 py-1.5 text-sm bg-(--color-surface-3) border border-(--color-border) text-(--color-text-secondary) hover:text-(--color-text-primary)">Search</button></form></div> <!></div>`);function Sc(e,t){He(t,!0);let r=F(ve([])),a=F(""),n=F(!0);async function o(){var S;const x=s(a)?`?query=${encodeURIComponent(s(a))}&limit=100`:"?limit=100",f=await Ke("/admin/bans"+x);f.status===200&&T(r,((S=f.data)==null?void 0:S.bans)||[],!0),T(n,!1)}Be(()=>{o()});async function i(x){confirm(`Unban ${x}?`)&&(await hr("/moderation/actions",{action:"unban",target:x}),o())}function l(x){if(x===-2)return"Permanent";if(x===0)return"Invalidated";const f=Math.floor(x/3600),S=Math.floor(x%3600/60);return f>0?`${f}h ${S}m`:`${S}m`}function c(x){return x?new Date(x*1e3).toLocaleString():""}var d=kc(),v=u(d),m=b(u(v),2),_=u(m),g=b(v,2);{var h=x=>{var f=gc();y(x,f)},$=x=>{var f=$c(),S=u(f),N=u(S),k=b(u(N));Ee(k,21,()=>s(r),Se,(M,C)=>{var I=yc(),D=u(I),O=u(D),H=b(D),z=u(H),U=b(H),G=u(U),B=b(U),w=u(B),L=b(B),W=u(L),A=u(W),R=b(L),Z=u(R);j((ne,ue)=>{P(O,s(C).ipid),P(z,s(C).reason),P(G,s(C).moderator),P(w,ne),xe(W,1,`text-[10px] px-1 py-px font-medium ${s(C).permanent?"bg-red-500/15 text-red-400":"bg-(--color-surface-3) text-(--color-text-muted)"}`),P(A,ue)},[()=>c(s(C).timestamp),()=>l(s(C).duration)]),re("click",Z,()=>i(s(C).ipid)),y(M,I)},M=>{var C=wc();y(M,C)}),y(x,f)};q(g,x=>{s(n)?x(h):x($,-1)})}Po("submit",m,x=>{x.preventDefault(),o()}),Pr(_,()=>s(a),x=>T(a,x)),y(e,d),Ve()}zt(["click"]);var Ec=E('<p class="text-xs mt-2 text-(--color-text-muted)"> </p>'),Ac=E('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Nc=E('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-3 py-1.5 text-(--color-text-muted)"> </td><td class="px-3 py-1.5 font-mono"> </td><td class="px-3 py-1.5 text-(--color-text-secondary)"> </td><td class="px-3 py-1.5"><span> </span></td><td class="px-3 py-1.5 text-(--color-text-secondary) max-w-xs truncate hidden md:table-cell"> </td><td class="px-3 py-1.5 text-(--color-text-muted) max-w-xs truncate hidden lg:table-cell"> </td></tr>'),Tc=E('<tr><td colspan="6" class="px-4 py-8 text-center text-(--color-text-muted)">No events</td></tr>'),Mc=E('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-xs"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-3 py-2">Time</th><th class="px-3 py-2">IPID</th><th class="px-3 py-2">Ch</th><th class="px-3 py-2">Action</th><th class="px-3 py-2 hidden md:table-cell">Message</th><th class="px-3 py-2 hidden lg:table-cell">Reason</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),Pc=E('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-mono text-xs"> </td><td class="px-4 py-1.5 text-(--color-text-secondary)"> </td><td class="px-4 py-1.5"> </td><td class="px-4 py-1.5"><button class="text-xs text-red-400 hover:text-red-300">Unmute</button></td></tr>'),Ic=E('<tr><td colspan="4" class="px-4 py-8 text-center text-(--color-text-muted)">No active mutes</td></tr>'),Oc=E('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">IPID</th><th class="px-4 py-2">Reason</th><th class="px-4 py-2">Remaining</th><th class="px-4 py-2"></th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),Cc=E('<div class="space-y-4"><h2 class="text-lg font-semibold">Moderation</h2> <div class="bg-(--color-surface-1) border border-(--color-border) p-4"><h3 class="text-[10px] font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3">Quick Action</h3> <div class="flex flex-wrap gap-px"><input placeholder="IPID / session ID / IP" class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) placeholder:text-(--color-text-muted) w-44 focus:outline-none focus:border-(--color-border-active)"/> <input placeholder="Reason" class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) placeholder:text-(--color-text-muted) flex-1 min-w-24 focus:outline-none focus:border-(--color-border-active)"/> <button class="px-3 py-1.5 text-sm bg-amber-600 text-white hover:bg-amber-500">Kick</button> <button class="px-3 py-1.5 text-sm bg-orange-600 text-white hover:bg-orange-500">Mute</button> <button class="px-3 py-1.5 text-sm bg-red-600 text-white hover:bg-red-500">Ban</button></div> <!></div> <div class="flex gap-4 border-b border-(--color-border)"><button>Audit Log</button> <button>Active Mutes</button></div> <!></div>');function Lc(e,t){He(t,!0);let r=F("audit"),a=F(ve([])),n=F(ve([])),o=F(!0),i=F(""),l=F(""),c=F("");const d={0:"none",1:"log",2:"censor",3:"drop",4:"mute",5:"kick",6:"ban",7:"perma_ban",none:"none",log:"log",censor:"censor",drop:"drop",mute:"mute",kick:"kick",ban:"ban",perma_ban:"perma_ban"},v={ban:"bg-red-500/15 text-red-400",perma_ban:"bg-red-500/15 text-red-400",kick:"bg-amber-500/15 text-amber-400",mute:"bg-orange-500/15 text-orange-400",censor:"bg-violet-500/15 text-violet-400",drop:"bg-violet-500/15 text-violet-400",log:"bg-(--color-surface-3) text-(--color-text-muted)",none:"bg-(--color-surface-3) text-(--color-text-muted)"};function m(A){return d[String(A)]||String(A)}function _(A){return v[m(A)]||"bg-(--color-surface-3) text-(--color-text-muted)"}async function g(){var R;T(o,!0);const A=await Ke("/admin/moderation-events?limit=200");A.status===200&&T(a,((R=A.data)==null?void 0:R.events)||[],!0),T(o,!1)}async function h(){var R;T(o,!0);const A=await Ke("/admin/mutes");A.status===200&&T(n,((R=A.data)==null?void 0:R.mutes)||[],!0),T(o,!1)}Be(()=>{s(r)==="audit"?g():h()});async function $(A){var ne;if(!s(i))return;T(c,"");const R={action:A,target:s(i),reason:s(l)||void 0};A==="mute"&&(R.duration=900);const Z=await hr("/moderation/actions",R);T(c,Z.status===200?`${A} applied`:((ne=Z.data)==null?void 0:ne.reason)||"Failed",!0),T(i,""),T(l,""),s(r)==="mutes"&&h()}async function x(A){await hr("/moderation/actions",{action:"unmute",target:A}),h()}var f=Cc(),S=b(u(f),2),N=b(u(S),2),k=u(N),M=b(k,2),C=b(M,2),I=b(C,2),D=b(I,2),O=b(N,2);{var H=A=>{var R=Ec(),Z=u(R);j(()=>P(Z,s(c))),y(A,R)};q(O,A=>{s(c)&&A(H)})}var z=b(S,2),U=u(z),G=b(U,2),B=b(z,2);{var w=A=>{var R=Ac();y(A,R)},L=A=>{var R=Mc(),Z=u(R),ne=u(Z),ue=b(u(ne));Ee(ue,21,()=>s(a),Se,(X,Y)=>{var se=Nc(),ie=u(se),oe=u(ie),Me=b(ie),ut=u(Me),ge=b(Me),Re=u(ge),he=b(ge),ye=u(he),ft=u(ye),Ge=b(he),mt=u(Ge),gt=b(Ge),yt=u(gt);j((Zt,jt,et)=>{P(oe,Zt),P(ut,s(Y).ipid),P(Re,s(Y).channel),xe(ye,1,`text-[10px] px-1 py-px font-medium ${jt??""}`),P(ft,et),P(mt,s(Y).message_sample),P(yt,s(Y).reason)},[()=>new Date(s(Y).timestamp_ms).toLocaleString(),()=>_(s(Y).action),()=>m(s(Y).action)]),y(X,se)},X=>{var Y=Tc();y(X,Y)}),y(A,R)},W=A=>{var R=Oc(),Z=u(R),ne=u(Z),ue=b(u(ne));Ee(ue,21,()=>s(n),Se,(X,Y)=>{var se=Pc(),ie=u(se),oe=u(ie),Me=b(ie),ut=u(Me),ge=b(Me),Re=u(ge),he=b(ge),ye=u(he);j(ft=>{P(oe,s(Y).ipid),P(ut,s(Y).reason),P(Re,ft)},[()=>s(Y).seconds_remaining<0?"Permanent":Math.ceil(s(Y).seconds_remaining/60)+"m"]),re("click",ye,()=>x(s(Y).ipid)),y(X,se)},X=>{var Y=Ic();y(X,Y)}),y(A,R)};q(B,A=>{s(o)?A(w):s(r)==="audit"?A(L,1):A(W,-1)})}j(()=>{xe(U,1,`pb-2 text-sm border-b-2 transition-colors ${s(r)==="audit"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),xe(G,1,`pb-2 text-sm border-b-2 transition-colors ${s(r)==="mutes"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`)}),Pr(k,()=>s(i),A=>T(i,A)),Pr(M,()=>s(l),A=>T(l,A)),re("click",C,()=>$("kick")),re("click",I,()=>$("mute")),re("click",D,()=>$("ban")),re("click",U,()=>T(r,"audit")),re("click",G,()=>T(r,"mutes")),y(e,f),Ve()}zt(["click"]);var Rc=E('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Dc=E('<span class="px-1 py-px bg-cyan-500/15 text-cyan-400 font-medium"> </span>'),zc=E('<span class="px-1 py-px bg-(--color-surface-3) text-(--color-text-muted)">IDLE</span>'),jc=E('<span class="px-1 py-px bg-amber-500/15 text-amber-400 font-medium"> </span>'),Fc=E('<button><div class="flex items-center justify-between mb-1"><span class="font-medium text-sm"> </span> <span class="text-xs text-(--color-text-muted) tabular-nums"> </span></div> <div class="flex gap-1 text-[10px]"><!> <!></div></button>'),Bc=E('<div><span class="text-(--color-text-muted) text-[10px] uppercase">Background</span><div class="mt-0.5"> </div></div>'),Hc=E('<div><span class="text-(--color-text-muted) text-[10px] uppercase">Music</span><div class="mt-0.5"> </div></div>'),Vc=E('<div><span class="text-(--color-text-muted) text-[10px] uppercase">HP</span><div class="mt-0.5"> </div></div>'),Uc=E('<div class="bg-(--color-surface-1) border border-(--color-border) p-4 space-y-3"><h3 class="text-sm font-semibold"> </h3> <div class="grid grid-cols-2 sm:grid-cols-3 gap-3 text-xs"><!> <!> <!></div></div>'),Wc=E('<div class="grid gap-px bg-(--color-border) sm:grid-cols-2 lg:grid-cols-3"></div> <!>',1),qc=E('<div class="space-y-4"><h2 class="text-lg font-semibold">Areas</h2> <!></div>');function Kc(e,t){He(t,!0);let r=F(ve([])),a=F(!0),n=F(null);async function o(){var _;const m=await Ke("/areas");m.status===200&&T(r,((_=m.data)==null?void 0:_.areas)||[],!0),T(a,!1)}Be(()=>{o();const m=setInterval(o,1e4);return()=>clearInterval(m)});async function i(m){const _=await Ke(`/areas/${m}`);_.status===200&&T(n,_.data,!0)}var l=qc(),c=b(u(l),2);{var d=m=>{var _=Rc();y(m,_)},v=m=>{var _=Wc(),g=te(_);Ee(g,21,()=>s(r),Se,(x,f)=>{var S=Fc(),N=u(S),k=u(N),M=u(k),C=b(k,2),I=u(C),D=b(N,2),O=u(D);{var H=B=>{var w=Dc(),L=u(w);j(()=>P(L,s(f).status)),y(B,w)},z=B=>{var w=zc();y(B,w)};q(O,B=>{s(f).status&&s(f).status!=="IDLE"?B(H):B(z,-1)})}var U=b(O,2);{var G=B=>{var w=jc(),L=u(w);j(()=>P(L,s(f).locked)),y(B,w)};q(U,B=>{s(f).locked&&s(f).locked!=="FREE"&&B(G)})}j(()=>{var B,w;xe(S,1,`bg-(--color-surface-1) p-3 text-left hover:bg-(--color-surface-2) transition-colors
                 ${((w=(B=s(n))==null?void 0:B.area)==null?void 0:w.id)===s(f).id?"ring-1 ring-(--color-accent) ring-inset":""}`),P(M,s(f).name),P(I,s(f).players??0)}),re("click",S,()=>i(s(f).id)),y(x,S)});var h=b(g,2);{var $=x=>{var f=Uc(),S=u(f),N=u(S),k=b(S,2),M=u(k);{var C=z=>{var U=Bc(),G=b(u(U)),B=u(G);j(()=>P(B,s(n).background.name)),y(z,U)};q(M,z=>{s(n).background&&z(C)})}var I=b(M,2);{var D=z=>{var U=Hc(),G=b(u(U)),B=u(G);j(()=>P(B,s(n).music.name||"None")),y(z,U)};q(I,z=>{s(n).music&&z(D)})}var O=b(I,2);{var H=z=>{var U=Vc(),G=b(u(U)),B=u(G);j(()=>P(B,`Def ${s(n).hp.defense??""} / Pro ${s(n).hp.prosecution??""}`)),y(z,U)};q(O,z=>{s(n).hp&&z(H)})}j(()=>{var z;return P(N,(z=s(n).area)==null?void 0:z.name)}),y(x,f)};q(h,x=>{s(n)&&x($)})}y(m,_)};q(c,m=>{s(a)?m(d):m(v,-1)})}y(e,l),Ve()}zt(["click"]);var Gc=E('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Yc=E('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-medium"> </td><td class="px-4 py-1.5"><span> </span></td></tr>'),Jc=E('<tr><td colspan="2" class="px-4 py-8 text-center text-(--color-text-muted)">No accounts</td></tr>'),Zc=E('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">Username</th><th class="px-4 py-2">Role</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),Xc=E('<div class="space-y-4"><h2 class="text-lg font-semibold">Accounts</h2> <!></div>');function Qc(e,t){He(t,!0);let r=F(ve([])),a=F(!0);async function n(){var v;const d=await Ke("/admin/users");d.status===200&&T(r,((v=d.data)==null?void 0:v.users)||[],!0),T(a,!1)}Be(()=>{n()});var o=Xc(),i=b(u(o),2);{var l=d=>{var v=Gc();y(d,v)},c=d=>{var v=Zc(),m=u(v),_=u(m),g=b(u(_));Ee(g,21,()=>s(r),Se,(h,$)=>{var x=Yc(),f=u(x),S=u(f),N=b(f),k=u(N),M=u(k);j(()=>{P(S,s($).username),xe(k,1,`text-[10px] px-1 py-px font-medium
                    ${s($).acl==="SUPER"?"bg-violet-500/15 text-violet-400":s($).acl==="NONE"?"bg-(--color-surface-3) text-(--color-text-muted)":"bg-cyan-500/15 text-cyan-400"}`),P(M,s($).acl)}),y(h,x)},h=>{var $=Jc();y(h,$)}),y(d,v)};q(i,d=>{s(a)?d(l):d(c,-1)})}y(e,o),Ve()}var ed=E('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),td=E('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-3 py-1.5 font-mono text-xs"> </td><td class="px-3 py-1.5 text-(--color-text-secondary) max-w-xs truncate"> </td><td class="px-3 py-1.5 text-xs"> </td></tr>'),rd=E('<table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-3 py-2">Target</th><th class="px-3 py-2">Reason</th><th class="px-3 py-2">Expires</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table>'),ad=E('<p class="text-sm text-(--color-text-muted)">No active rules.</p>'),nd=E('<div class="bg-(--color-surface-1) border border-(--color-border) p-4"><div class="flex items-center gap-2 mb-3"><span class="text-sm font-medium">nftables</span> <span> </span> <span class="text-xs text-(--color-text-muted)"> </span></div> <!></div>'),od=E('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-mono text-xs"> </td><td class="px-4 py-1.5"> </td><td class="px-4 py-1.5"><span> </span></td><td class="px-4 py-1.5 text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td><td class="px-4 py-1.5 text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td></tr>'),sd=E('<tr><td colspan="5" class="px-4 py-8 text-center text-(--color-text-muted)">No flagged ASNs</td></tr>'),id=E('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">ASN</th><th class="px-4 py-2">Organization</th><th class="px-4 py-2">Status</th><th class="px-4 py-2 hidden md:table-cell">Events</th><th class="px-4 py-2 hidden md:table-cell">IPs</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),ld=E('<div class="space-y-4"><h2 class="text-lg font-semibold">Firewall</h2> <div class="flex gap-4 border-b border-(--color-border)"><button>IP Rules</button> <button>ASN Reputation</button></div> <!></div>');function cd(e,t){He(t,!0);let r=F("firewall"),a=F(ve({enabled:!1,rules:[]})),n=F(ve([])),o=F(!0);async function i(){T(o,!0);const f=await Ke("/admin/firewall");f.status===200&&T(a,f.data,!0),T(o,!1)}async function l(){var S;T(o,!0);const f=await Ke("/admin/asn-reputation");f.status===200&&T(n,((S=f.data)==null?void 0:S.asn_entries)||[],!0),T(o,!1)}Be(()=>{s(r)==="firewall"?i():l()});function c(f){return f?new Date(f*1e3).toLocaleString():"Never"}var d=ld(),v=b(u(d),2),m=u(v),_=b(m,2),g=b(v,2);{var h=f=>{var S=ed();y(f,S)},$=f=>{var S=nd(),N=u(S),k=b(u(N),2),M=u(k),C=b(k,2),I=u(C),D=b(N,2);{var O=z=>{var U=rd(),G=b(u(U));Ee(G,21,()=>s(a).rules,Se,(B,w)=>{var L=td(),W=u(L),A=u(W),R=b(W),Z=u(R),ne=b(R),ue=u(ne);j(X=>{P(A,s(w).target),P(Z,s(w).reason),P(ue,X)},[()=>s(w).expires_at===0?"Permanent":c(s(w).expires_at)]),y(B,L)}),y(z,U)},H=z=>{var U=ad();y(z,U)};q(D,z=>{s(a).rules.length>0?z(O):z(H,-1)})}j(()=>{xe(k,1,`text-[10px] px-1 py-px font-medium ${s(a).enabled?"bg-emerald-500/15 text-emerald-400":"bg-(--color-surface-3) text-(--color-text-muted)"}`),P(M,s(a).enabled?"Enabled":"Disabled"),P(I,`${s(a).rules.length??""} rules`)}),y(f,S)},x=f=>{var S=id(),N=u(S),k=u(N),M=b(u(k));Ee(M,21,()=>s(n),Se,(C,I)=>{var D=od(),O=u(D),H=u(O),z=b(O),U=u(z),G=b(z),B=u(G),w=u(B),L=b(G),W=u(L),A=b(L),R=u(A);j(()=>{P(H,`AS${s(I).asn??""}`),P(U,s(I).as_org),xe(B,1,`text-[10px] px-1 py-px font-medium
                    ${s(I).status==="blocked"?"bg-red-500/15 text-red-400":s(I).status==="rate_limited"?"bg-orange-500/15 text-orange-400":s(I).status==="watched"?"bg-amber-500/15 text-amber-400":"bg-(--color-surface-3) text-(--color-text-muted)"}`),P(w,s(I).status),P(W,s(I).total_abuse_events),P(R,s(I).abusive_ips)}),y(C,D)},C=>{var I=sd();y(C,I)}),y(f,S)};q(g,f=>{s(o)?f(h):s(r)==="firewall"?f($,1):f(x,-1)})}j(()=>{xe(m,1,`pb-2 text-sm border-b-2 transition-colors ${s(r)==="firewall"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),xe(_,1,`pb-2 text-sm border-b-2 transition-colors ${s(r)==="asn"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`)}),re("click",m,()=>T(r,"firewall")),re("click",_,()=>T(r,"asn")),y(e,d),Ve()}zt(["click"]);var dd=E('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),ud=E('<div class="px-4 py-2 bg-(--color-surface-2) text-xs font-semibold uppercase tracking-wider text-(--color-text-secondary) border-b border-(--color-border) sticky top-0"> </div>'),fd=E('<div class="px-4 py-1.5 flex items-center gap-3 text-sm hover:bg-(--color-surface-2)/50 border-b border-(--color-border)/30"><span class="text-[10px] text-(--color-text-muted) w-6 text-right tabular-nums"></span> <span> </span></div>'),vd=E('<div class="px-4 py-8 text-center text-(--color-text-muted) text-sm">No items</div>'),pd=E('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="max-h-[65vh] overflow-y-auto"></div></div>'),hd=E('<div class="space-y-4"><h2 class="text-lg font-semibold">Content</h2> <div class="flex gap-4 border-b border-(--color-border)"><button> </button> <button> </button> <button> </button></div> <!></div>');function _d(e,t){He(t,!0);let r=F("areas"),a=F(ve({characters:[],music:[],areas:[]})),n=F(!0);async function o(){const N=await Ke("/admin/content");N.status===200&&T(a,N.data,!0),T(n,!1)}Be(()=>{o()});function i(){return s(r)==="areas"?s(a).areas:s(r)==="characters"?s(a).characters:s(r)==="music"?s(a).music:[]}function l(N){return s(r)==="music"&&!N.includes(".")}var c=hd(),d=b(u(c),2),v=u(d),m=u(v),_=b(v,2),g=u(_),h=b(_,2),$=u(h),x=b(d,2);{var f=N=>{var k=dd();y(N,k)},S=N=>{var k=pd(),M=u(k);Ee(M,21,i,Se,(C,I,D)=>{var O=ce(),H=te(O);{var z=B=>{var w=ud(),L=u(w);j(()=>P(L,s(I))),y(B,w)},U=de(()=>l(s(I))),G=B=>{var w=fd(),L=u(w);L.textContent=D+1;var W=b(L,2),A=u(W);j(()=>P(A,s(I))),y(B,w)};q(H,B=>{s(U)?B(z):B(G,-1)})}y(C,O)},C=>{var I=vd();y(C,I)}),y(N,k)};q(x,N=>{s(n)?N(f):N(S,-1)})}j(()=>{xe(v,1,`pb-2 text-sm border-b-2 transition-colors ${s(r)==="areas"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),P(m,`Areas (${s(a).areas.length??""})`),xe(_,1,`pb-2 text-sm border-b-2 transition-colors ${s(r)==="characters"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),P(g,`Characters (${s(a).characters.length??""})`),xe(h,1,`pb-2 text-sm border-b-2 transition-colors ${s(r)==="music"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),P($,`Music (${s(a).music.length??""})`)}),re("click",v,()=>T(r,"areas")),re("click",_,()=>T(r,"characters")),re("click",h,()=>T(r,"music")),y(e,c),Ve()}zt(["click"]);var xd=E('<div class="text-center text-gray-500 mt-20"><h2 class="text-2xl font-semibold mb-2">Coming Soon</h2> <p>The <code class="text-gray-400"> </code> page is not yet implemented.</p></div>'),bd=E('<div class="flex h-screen bg-(--color-surface-0) text-(--color-text-primary)"><!> <main class="flex-1 overflow-auto p-4 md:p-6 lg:p-8"><!></main></div>');function md(e,t){He(t,!0);let r=F(ve(window.location.hash||"#/login"));function a(){T(r,window.location.hash||"#/login",!0)}Be(()=>(window.addEventListener("hashchange",a),()=>window.removeEventListener("hashchange",a))),Be(()=>(ae.loggedIn?Zi():In(),()=>In()));let n=de(()=>!ae.loggedIn&&s(r)!=="#/login"?(window.location.hash="#/login","login"):ae.loggedIn&&s(r)==="#/login"?(window.location.hash="#/dashboard","dashboard"):s(r).slice(2)||"login");var o=ce(),i=te(o);{var l=d=>{Sl(d,{})},c=d=>{var v=bd(),m=u(v);wl(m,{get currentPage(){return s(n)}});var _=b(m,2),g=u(_);{var h=O=>{zl(O,{})},$=O=>{ic(O,{})},x=O=>{pc(O,{})},f=O=>{mc(O,{})},S=O=>{Sc(O,{})},N=O=>{Lc(O,{})},k=O=>{Kc(O,{})},M=O=>{Qc(O,{})},C=O=>{cd(O,{})},I=O=>{_d(O,{})},D=O=>{var H=xd(),z=b(u(H),2),U=b(u(z)),G=u(U);j(()=>P(G,s(n))),y(O,H)};q(g,O=>{s(n)==="dashboard"?O(h):s(n)==="config"?O($,1):s(n)==="sessions"?O(x,2):s(n)==="traffic"?O(f,3):s(n)==="bans"?O(S,4):s(n)==="moderation"?O(N,5):s(n)==="areas"?O(k,6):s(n)==="users"?O(M,7):s(n)==="firewall"?O(C,8):s(n)==="content"?O(I,9):O(D,-1)})}y(d,v)};q(i,d=>{ae.loggedIn?d(c,-1):d(l)})}y(e,o),Ve()}$i(md,{target:document.getElementById("app")});
