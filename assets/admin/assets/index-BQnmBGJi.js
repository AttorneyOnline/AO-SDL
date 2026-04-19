var ts=Object.defineProperty;var No=e=>{throw TypeError(e)};var rs=(e,t,r)=>t in e?ts(e,t,{enumerable:!0,configurable:!0,writable:!0,value:r}):e[t]=r;var yt=(e,t,r)=>rs(e,typeof t!="symbol"?t+"":t,r),Fa=(e,t,r)=>t.has(e)||No("Cannot "+r);var h=(e,t,r)=>(Fa(e,t,"read from private field"),r?r.call(e):t.get(e)),X=(e,t,r)=>t.has(e)?No("Cannot add the same private member more than once"):t instanceof WeakSet?t.add(e):t.set(e,r),re=(e,t,r,a)=>(Fa(e,t,"write to private field"),a?a.call(e,r):t.set(e,r),r),he=(e,t,r)=>(Fa(e,t,"access private method"),r);(function(){const t=document.createElement("link").relList;if(t&&t.supports&&t.supports("modulepreload"))return;for(const o of document.querySelectorAll('link[rel="modulepreload"]'))a(o);new MutationObserver(o=>{for(const s of o)if(s.type==="childList")for(const i of s.addedNodes)i.tagName==="LINK"&&i.rel==="modulepreload"&&a(i)}).observe(document,{childList:!0,subtree:!0});function r(o){const s={};return o.integrity&&(s.integrity=o.integrity),o.referrerPolicy&&(s.referrerPolicy=o.referrerPolicy),o.crossOrigin==="use-credentials"?s.credentials="include":o.crossOrigin==="anonymous"?s.credentials="omit":s.credentials="same-origin",s}function a(o){if(o.ep)return;o.ep=!0;const s=r(o);fetch(o.href,s)}})();const as="5";var Go;typeof window<"u"&&((Go=window.__svelte??(window.__svelte={})).v??(Go.v=new Set)).add(as);const os=1,ns=2,Xo=4,ss=8,is=16,ls=1,cs=2,Qo=4,ds=8,us=16,fs=1,vs=2,Le=Symbol(),en="http://www.w3.org/1999/xhtml",ps="http://www.w3.org/2000/svg",hs="@attach",_s=!1;var vo=Array.isArray,xs=Array.prototype.indexOf,Rr=Array.prototype.includes,Oa=Array.from,bs=Object.defineProperty,er=Object.getOwnPropertyDescriptor,tn=Object.getOwnPropertyDescriptors,ms=Object.prototype,gs=Array.prototype,po=Object.getPrototypeOf,To=Object.isExtensible;function qr(e){return typeof e=="function"}const gr=()=>{};function ys(e){return e()}function Ja(e){for(var t=0;t<e.length;t++)e[t]()}function rn(){var e,t,r=new Promise((a,o)=>{e=a,t=o});return{promise:r,resolve:e,reject:t}}function ho(e,t){if(Array.isArray(e))return e;if(!(Symbol.iterator in e))return Array.from(e);const r=[];for(const a of e)if(r.push(a),r.length===t)break;return r}const Ve=2,Dr=4,ca=8,_o=1<<24,Pt=16,St=32,rr=64,Ga=128,pt=512,Re=1024,Be=2048,Mt=4096,qe=8192,lt=16384,Er=32768,Io=1<<25,ar=65536,Ya=1<<17,ws=1<<18,Vr=1<<19,an=1<<20,It=1<<25,$r=65536,Za=1<<21,ea=1<<22,tr=1<<23,Bt=Symbol("$state"),on=Symbol("legacy props"),ks=Symbol(""),Rt=new class extends Error{constructor(){super(...arguments);yt(this,"name","StaleReactionError");yt(this,"message","The reaction that called `getAbortSignal()` was re-run or destroyed")}};var Yo;const xo=!!((Yo=globalThis.document)!=null&&Yo.contentType)&&globalThis.document.contentType.includes("xml");function $s(){throw new Error("https://svelte.dev/e/async_derived_orphan")}function Ss(e,t,r){throw new Error("https://svelte.dev/e/each_key_duplicate")}function Es(e){throw new Error("https://svelte.dev/e/effect_in_teardown")}function As(){throw new Error("https://svelte.dev/e/effect_in_unowned_derived")}function Ns(e){throw new Error("https://svelte.dev/e/effect_orphan")}function Ts(){throw new Error("https://svelte.dev/e/effect_update_depth_exceeded")}function Is(e){throw new Error("https://svelte.dev/e/props_invalid_value")}function Ps(){throw new Error("https://svelte.dev/e/state_descriptors_fixed")}function Ms(){throw new Error("https://svelte.dev/e/state_prototype_fixed")}function Os(){throw new Error("https://svelte.dev/e/state_unsafe_mutation")}function Cs(){throw new Error("https://svelte.dev/e/svelte_boundary_reset_onerror")}function Ls(){console.warn("https://svelte.dev/e/derived_inert")}function Rs(){console.warn("https://svelte.dev/e/select_multiple_invalid_value")}function Ds(){console.warn("https://svelte.dev/e/svelte_boundary_reset_noop")}function nn(e){return e===this.v}function zs(e,t){return e!=e?t==t:e!==t||e!==null&&typeof e=="object"||typeof e=="function"}function sn(e){return!zs(e,this.v)}let da=!1,js=!1;function Fs(){da=!0}let ze=null;function zr(e){ze=e}function Je(e,t=!1,r){ze={p:ze,i:!1,c:null,e:null,s:e,x:null,r:te,l:da&&!t?{s:null,u:null,$:[]}:null}}function Ge(e){var t=ze,r=t.e;if(r!==null){t.e=null;for(var a of r)Nn(a)}return t.i=!0,ze=t.p,{}}function ua(){return!da||ze!==null&&ze.l===null}let fr=[];function ln(){var e=fr;fr=[],Ja(e)}function Wt(e){if(fr.length===0&&!Zr){var t=fr;queueMicrotask(()=>{t===fr&&ln()})}fr.push(e)}function Bs(){for(;fr.length>0;)ln()}function cn(e){var t=te;if(t===null)return ne.f|=tr,e;if((t.f&Er)===0&&(t.f&Dr)===0)throw e;Qt(e,t)}function Qt(e,t){for(;t!==null;){if((t.f&Ga)!==0){if((t.f&Er)===0)throw e;try{t.b.error(e);return}catch(r){e=r}}t=t.parent}throw e}const Ws=-7169;function Ee(e,t){e.f=e.f&Ws|t}function bo(e){(e.f&pt)!==0||e.deps===null?Ee(e,Re):Ee(e,Mt)}function dn(e){if(e!==null)for(const t of e)(t.f&Ve)===0||(t.f&$r)===0||(t.f^=$r,dn(t.deps))}function un(e,t,r){(e.f&Be)!==0?t.add(e):(e.f&Mt)!==0&&r.add(e),dn(e.deps),Ee(e,Re)}let ma=!1;function Vs(e){var t=ma;try{return ma=!1,[e(),ma]}finally{ma=t}}const cr=new Set;let U=null,Fe=null,Xa=null,Zr=!1,Ba=!1,Ir=null,ya=null;var Po=0;let Hs=1;var Pr,Mr,hr,Dt,At,aa,nt,oa,Zt,zt,Nt,Or,Cr,_r,Pe,wa,fn,ka,Qa,$a,Us;const Ia=class Ia{constructor(){X(this,Pe);yt(this,"id",Hs++);yt(this,"current",new Map);yt(this,"previous",new Map);X(this,Pr,new Set);X(this,Mr,new Set);X(this,hr,new Set);X(this,Dt,new Map);X(this,At,new Map);X(this,aa,null);X(this,nt,[]);X(this,oa,[]);X(this,Zt,new Set);X(this,zt,new Set);X(this,Nt,new Map);X(this,Or,new Set);yt(this,"is_fork",!1);X(this,Cr,!1);X(this,_r,new Set)}skip_effect(t){h(this,Nt).has(t)||h(this,Nt).set(t,{d:[],m:[]}),h(this,Or).delete(t)}unskip_effect(t,r=a=>this.schedule(a)){var a=h(this,Nt).get(t);if(a){h(this,Nt).delete(t);for(var o of a.d)Ee(o,Be),r(o);for(o of a.m)Ee(o,Mt),r(o)}h(this,Or).add(t)}capture(t,r,a=!1){t.v!==Le&&!this.previous.has(t)&&this.previous.set(t,t.v),(t.f&tr)===0&&(this.current.set(t,[r,a]),Fe==null||Fe.set(t,r)),this.is_fork||(t.v=r)}activate(){U=this}deactivate(){U=null,Fe=null}flush(){try{Ba=!0,U=this,he(this,Pe,ka).call(this)}finally{Po=0,Xa=null,Ir=null,ya=null,Ba=!1,U=null,Fe=null,yr.clear()}}discard(){for(const t of h(this,Mr))t(this);h(this,Mr).clear(),h(this,hr).clear(),cr.delete(this)}register_created_effect(t){h(this,oa).push(t)}increment(t,r){let a=h(this,Dt).get(r)??0;if(h(this,Dt).set(r,a+1),t){let o=h(this,At).get(r)??0;h(this,At).set(r,o+1)}}decrement(t,r,a){let o=h(this,Dt).get(r)??0;if(o===1?h(this,Dt).delete(r):h(this,Dt).set(r,o-1),t){let s=h(this,At).get(r)??0;s===1?h(this,At).delete(r):h(this,At).set(r,s-1)}h(this,Cr)||a||(re(this,Cr,!0),Wt(()=>{re(this,Cr,!1),this.flush()}))}transfer_effects(t,r){for(const a of t)h(this,Zt).add(a);for(const a of r)h(this,zt).add(a);t.clear(),r.clear()}oncommit(t){h(this,Pr).add(t)}ondiscard(t){h(this,Mr).add(t)}on_fork_commit(t){h(this,hr).add(t)}run_fork_commit_callbacks(){for(const t of h(this,hr))t(this);h(this,hr).clear()}settled(){return(h(this,aa)??re(this,aa,rn())).promise}static ensure(){if(U===null){const t=U=new Ia;Ba||(cr.add(U),Zr||Wt(()=>{U===t&&t.flush()}))}return U}apply(){{Fe=null;return}}schedule(t){var o;if(Xa=t,(o=t.b)!=null&&o.is_pending&&(t.f&(Dr|ca|_o))!==0&&(t.f&Er)===0){t.b.defer_effect(t);return}for(var r=t;r.parent!==null;){r=r.parent;var a=r.f;if(Ir!==null&&r===te&&(ne===null||(ne.f&Ve)===0))return;if((a&(rr|St))!==0){if((a&Re)===0)return;r.f^=Re}}h(this,nt).push(r)}};Pr=new WeakMap,Mr=new WeakMap,hr=new WeakMap,Dt=new WeakMap,At=new WeakMap,aa=new WeakMap,nt=new WeakMap,oa=new WeakMap,Zt=new WeakMap,zt=new WeakMap,Nt=new WeakMap,Or=new WeakMap,Cr=new WeakMap,_r=new WeakMap,Pe=new WeakSet,wa=function(){return this.is_fork||h(this,At).size>0},fn=function(){for(const a of h(this,_r))for(const o of h(a,At).keys()){for(var t=!1,r=o;r.parent!==null;){if(h(this,Nt).has(r)){t=!0;break}r=r.parent}if(!t)return!0}return!1},ka=function(){var l,c;if(Po++>1e3&&(cr.delete(this),Ks()),!he(this,Pe,wa).call(this)){for(const d of h(this,Zt))h(this,zt).delete(d),Ee(d,Be),this.schedule(d);for(const d of h(this,zt))Ee(d,Mt),this.schedule(d)}const t=h(this,nt);re(this,nt,[]),this.apply();var r=Ir=[],a=[],o=ya=[];for(const d of t)try{he(this,Pe,Qa).call(this,d,r,a)}catch(f){throw hn(d),f}if(U=null,o.length>0){var s=Ia.ensure();for(const d of o)s.schedule(d)}if(Ir=null,ya=null,he(this,Pe,wa).call(this)||he(this,Pe,fn).call(this)){he(this,Pe,$a).call(this,a),he(this,Pe,$a).call(this,r);for(const[d,f]of h(this,Nt))pn(d,f)}else{h(this,Dt).size===0&&cr.delete(this),h(this,Zt).clear(),h(this,zt).clear();for(const d of h(this,Pr))d(this);h(this,Pr).clear(),Mo(a),Mo(r),(l=h(this,aa))==null||l.resolve()}var i=U;if(h(this,nt).length>0){const d=i??(i=this);h(d,nt).push(...h(this,nt).filter(f=>!h(d,nt).includes(f)))}i!==null&&(cr.add(i),he(c=i,Pe,ka).call(c))},Qa=function(t,r,a){t.f^=Re;for(var o=t.first;o!==null;){var s=o.f,i=(s&(St|rr))!==0,l=i&&(s&Re)!==0,c=l||(s&qe)!==0||h(this,Nt).has(o);if(!c&&o.fn!==null){i?o.f^=Re:(s&Dr)!==0?r.push(o):ha(o)&&((s&Pt)!==0&&h(this,zt).add(o),Br(o));var d=o.first;if(d!==null){o=d;continue}}for(;o!==null;){var f=o.next;if(f!==null){o=f;break}o=o.parent}}},$a=function(t){for(var r=0;r<t.length;r+=1)un(t[r],h(this,Zt),h(this,zt))},Us=function(){var f,g,b;for(const y of cr){var t=y.id<this.id,r=[];for(const[p,[$,x]]of this.current){if(y.current.has(p)){var a=y.current.get(p)[0];if(t&&$!==a)y.current.set(p,[$,x]);else continue}r.push(p)}var o=[...y.current.keys()].filter(p=>!this.current.has(p));if(o.length===0)t&&y.discard();else if(r.length>0){if(t)for(const p of h(this,Or))y.unskip_effect(p,$=>{var x;($.f&(Pt|ea))!==0?y.schedule($):he(x=y,Pe,$a).call(x,[$])});y.activate();var s=new Set,i=new Map;for(var l of r)vn(l,o,s,i);i=new Map;var c=[...y.current.keys()].filter(p=>this.current.has(p)?this.current.get(p)[0]!==p:!0);for(const p of h(this,oa))(p.f&(lt|qe|Ya))===0&&mo(p,c,i)&&((p.f&(ea|Pt))!==0?(Ee(p,Be),y.schedule(p)):h(y,Zt).add(p));if(h(y,nt).length>0){y.apply();for(var d of h(y,nt))he(f=y,Pe,Qa).call(f,d,[],[]);re(y,nt,[])}y.deactivate()}}for(const y of cr)h(y,_r).has(this)&&(h(y,_r).delete(this),h(y,_r).size===0&&!he(g=y,Pe,wa).call(g)&&(y.activate(),he(b=y,Pe,ka).call(b)))};let Sr=Ia;function qs(e){var t=Zr;Zr=!0;try{for(var r;;){if(Bs(),U===null)return r;U.flush()}}finally{Zr=t}}function Ks(){try{Ts()}catch(e){Qt(e,Xa)}}let wt=null;function Mo(e){var t=e.length;if(t!==0){for(var r=0;r<t;){var a=e[r++];if((a.f&(lt|qe))===0&&ha(a)&&(wt=new Set,Br(a),a.deps===null&&a.first===null&&a.nodes===null&&a.teardown===null&&a.ac===null&&On(a),(wt==null?void 0:wt.size)>0)){yr.clear();for(const o of wt){if((o.f&(lt|qe))!==0)continue;const s=[o];let i=o.parent;for(;i!==null;)wt.has(i)&&(wt.delete(i),s.push(i)),i=i.parent;for(let l=s.length-1;l>=0;l--){const c=s[l];(c.f&(lt|qe))===0&&Br(c)}}wt.clear()}}wt=null}}function vn(e,t,r,a){if(!r.has(e)&&(r.add(e),e.reactions!==null))for(const o of e.reactions){const s=o.f;(s&Ve)!==0?vn(o,t,r,a):(s&(ea|Pt))!==0&&(s&Be)===0&&mo(o,t,a)&&(Ee(o,Be),go(o))}}function mo(e,t,r){const a=r.get(e);if(a!==void 0)return a;if(e.deps!==null)for(const o of e.deps){if(Rr.call(t,o))return!0;if((o.f&Ve)!==0&&mo(o,t,r))return r.set(o,!0),!0}return r.set(e,!1),!1}function go(e){U.schedule(e)}function pn(e,t){if(!((e.f&St)!==0&&(e.f&Re)!==0)){(e.f&Be)!==0?t.d.push(e):(e.f&Mt)!==0&&t.m.push(e),Ee(e,Re);for(var r=e.first;r!==null;)pn(r,t),r=r.next}}function hn(e){Ee(e,Re);for(var t=e.first;t!==null;)hn(t),t=t.next}function Js(e){let t=0,r=or(0),a;return()=>{ko()&&(n(r),In(()=>(t===0&&(a=Wr(()=>e(()=>Xr(r)))),t+=1,()=>{Wt(()=>{t-=1,t===0&&(a==null||a(),a=void 0,Xr(r))})})))}}var Gs=ar|Vr;function Ys(e,t,r,a){new Zs(e,t,r,a)}var ut,fo,ft,xr,Qe,vt,Ue,st,jt,br,Xt,Lr,na,sa,Ft,Pa,we,Xs,Qs,ei,eo,Sa,Ea,to,ro;class Zs{constructor(t,r,a,o){X(this,we);yt(this,"parent");yt(this,"is_pending",!1);yt(this,"transform_error");X(this,ut);X(this,fo,null);X(this,ft);X(this,xr);X(this,Qe);X(this,vt,null);X(this,Ue,null);X(this,st,null);X(this,jt,null);X(this,br,0);X(this,Xt,0);X(this,Lr,!1);X(this,na,new Set);X(this,sa,new Set);X(this,Ft,null);X(this,Pa,Js(()=>(re(this,Ft,or(h(this,br))),()=>{re(this,Ft,null)})));var s;re(this,ut,t),re(this,ft,r),re(this,xr,i=>{var l=te;l.b=this,l.f|=Ga,a(i)}),this.parent=te.b,this.transform_error=o??((s=this.parent)==null?void 0:s.transform_error)??(i=>i),re(this,Qe,pa(()=>{he(this,we,eo).call(this)},Gs))}defer_effect(t){un(t,h(this,na),h(this,sa))}is_rendered(){return!this.is_pending&&(!this.parent||this.parent.is_rendered())}has_pending_snippet(){return!!h(this,ft).pending}update_pending_count(t,r){he(this,we,to).call(this,t,r),re(this,br,h(this,br)+t),!(!h(this,Ft)||h(this,Lr))&&(re(this,Lr,!0),Wt(()=>{re(this,Lr,!1),h(this,Ft)&&jr(h(this,Ft),h(this,br))}))}get_effect_pending(){return h(this,Pa).call(this),n(h(this,Ft))}error(t){if(!h(this,ft).onerror&&!h(this,ft).failed)throw t;U!=null&&U.is_fork?(h(this,vt)&&U.skip_effect(h(this,vt)),h(this,Ue)&&U.skip_effect(h(this,Ue)),h(this,st)&&U.skip_effect(h(this,st)),U.on_fork_commit(()=>{he(this,we,ro).call(this,t)})):he(this,we,ro).call(this,t)}}ut=new WeakMap,fo=new WeakMap,ft=new WeakMap,xr=new WeakMap,Qe=new WeakMap,vt=new WeakMap,Ue=new WeakMap,st=new WeakMap,jt=new WeakMap,br=new WeakMap,Xt=new WeakMap,Lr=new WeakMap,na=new WeakMap,sa=new WeakMap,Ft=new WeakMap,Pa=new WeakMap,we=new WeakSet,Xs=function(){try{re(this,vt,tt(()=>h(this,xr).call(this,h(this,ut))))}catch(t){this.error(t)}},Qs=function(t){const r=h(this,ft).failed;r&&re(this,st,tt(()=>{r(h(this,ut),()=>t,()=>()=>{})}))},ei=function(){const t=h(this,ft).pending;t&&(this.is_pending=!0,re(this,Ue,tt(()=>t(h(this,ut)))),Wt(()=>{var r=re(this,jt,document.createDocumentFragment()),a=Vt();r.append(a),re(this,vt,he(this,we,Ea).call(this,()=>tt(()=>h(this,xr).call(this,a)))),h(this,Xt)===0&&(h(this,ut).before(r),re(this,jt,null),wr(h(this,Ue),()=>{re(this,Ue,null)}),he(this,we,Sa).call(this,U))}))},eo=function(){try{if(this.is_pending=this.has_pending_snippet(),re(this,Xt,0),re(this,br,0),re(this,vt,tt(()=>{h(this,xr).call(this,h(this,ut))})),h(this,Xt)>0){var t=re(this,jt,document.createDocumentFragment());Eo(h(this,vt),t);const r=h(this,ft).pending;re(this,Ue,tt(()=>r(h(this,ut))))}else he(this,we,Sa).call(this,U)}catch(r){this.error(r)}},Sa=function(t){this.is_pending=!1,t.transfer_effects(h(this,na),h(this,sa))},Ea=function(t){var r=te,a=ne,o=ze;xt(h(this,Qe)),_t(h(this,Qe)),zr(h(this,Qe).ctx);try{return Sr.ensure(),t()}catch(s){return cn(s),null}finally{xt(r),_t(a),zr(o)}},to=function(t,r){var a;if(!this.has_pending_snippet()){this.parent&&he(a=this.parent,we,to).call(a,t,r);return}re(this,Xt,h(this,Xt)+t),h(this,Xt)===0&&(he(this,we,Sa).call(this,r),h(this,Ue)&&wr(h(this,Ue),()=>{re(this,Ue,null)}),h(this,jt)&&(h(this,ut).before(h(this,jt)),re(this,jt,null)))},ro=function(t){h(this,vt)&&(We(h(this,vt)),re(this,vt,null)),h(this,Ue)&&(We(h(this,Ue)),re(this,Ue,null)),h(this,st)&&(We(h(this,st)),re(this,st,null));var r=h(this,ft).onerror;let a=h(this,ft).failed;var o=!1,s=!1;const i=()=>{if(o){Ds();return}o=!0,s&&Cs(),h(this,st)!==null&&wr(h(this,st),()=>{re(this,st,null)}),he(this,we,Ea).call(this,()=>{he(this,we,eo).call(this)})},l=c=>{try{s=!0,r==null||r(c,i),s=!1}catch(d){Qt(d,h(this,Qe)&&h(this,Qe).parent)}a&&re(this,st,he(this,we,Ea).call(this,()=>{try{return tt(()=>{var d=te;d.b=this,d.f|=Ga,a(h(this,ut),()=>c,()=>i)})}catch(d){return Qt(d,h(this,Qe).parent),null}}))};Wt(()=>{var c;try{c=this.transform_error(t)}catch(d){Qt(d,h(this,Qe)&&h(this,Qe).parent);return}c!==null&&typeof c=="object"&&typeof c.then=="function"?c.then(l,d=>Qt(d,h(this,Qe)&&h(this,Qe).parent)):l(c)})};function _n(e,t,r,a){const o=ua()?fa:yo;var s=e.filter(b=>!b.settled);if(r.length===0&&s.length===0){a(t.map(o));return}var i=te,l=ti(),c=s.length===1?s[0].promise:s.length>1?Promise.all(s.map(b=>b.promise)):null;function d(b){l();try{a(b)}catch(y){(i.f&lt)===0&&Qt(y,i)}Na()}if(r.length===0){c.then(()=>d(t.map(o)));return}var f=xn();function g(){Promise.all(r.map(b=>ri(b))).then(b=>d([...t.map(o),...b])).catch(b=>Qt(b,i)).finally(()=>f())}c?c.then(()=>{l(),g(),Na()}):g()}function ti(){var e=te,t=ne,r=ze,a=U;return function(s=!0){xt(e),_t(t),zr(r),s&&(e.f&lt)===0&&(a==null||a.activate(),a==null||a.apply())}}function Na(e=!0){xt(null),_t(null),zr(null),e&&(U==null||U.deactivate())}function xn(){var e=te,t=e.b,r=U,a=t.is_rendered();return t.update_pending_count(1,r),r.increment(a,e),(o=!1)=>{t.update_pending_count(-1,r),r.decrement(a,e,o)}}function fa(e){var t=Ve|Be;return te!==null&&(te.f|=Vr),{ctx:ze,deps:null,effects:null,equals:nn,f:t,fn:e,reactions:null,rv:0,v:Le,wv:0,parent:te,ac:null}}function ri(e,t,r){let a=te;a===null&&$s();var o=void 0,s=or(Le),i=!ne,l=new Map;return _i(()=>{var y;var c=te,d=rn();o=d.promise;try{Promise.resolve(e()).then(d.resolve,d.reject).finally(Na)}catch(p){d.reject(p),Na()}var f=U;if(i){if((c.f&Er)!==0)var g=xn();if(a.b.is_rendered())(y=l.get(f))==null||y.reject(Rt),l.delete(f);else{for(const p of l.values())p.reject(Rt);l.clear()}l.set(f,d)}const b=(p,$=void 0)=>{if(g){var x=$===Rt;g(x)}if(!($===Rt||(c.f&lt)!==0)){if(f.activate(),$)s.f|=tr,jr(s,$);else{(s.f&tr)!==0&&(s.f^=tr),jr(s,p);for(const[v,A]of l){if(l.delete(v),v===f)break;A.reject(Rt)}}f.deactivate()}};d.promise.then(b,p=>b(null,p||"unknown"))}),La(()=>{for(const c of l.values())c.reject(Rt)}),new Promise(c=>{function d(f){function g(){f===o?c(s):d(o)}f.then(g,g)}d(o)})}function pe(e){const t=fa(e);return Rn(t),t}function yo(e){const t=fa(e);return t.equals=sn,t}function ai(e){var t=e.effects;if(t!==null){e.effects=null;for(var r=0;r<t.length;r+=1)We(t[r])}}function wo(e){var t,r=te,a=e.parent;if(!Ht&&a!==null&&(a.f&(lt|qe))!==0)return Ls(),e.v;xt(a);try{e.f&=~$r,ai(e),t=Fn(e)}finally{xt(r)}return t}function bn(e){var t=wo(e);if(!e.equals(t)&&(e.wv=zn(),(!(U!=null&&U.is_fork)||e.deps===null)&&(U!==null?U.capture(e,t,!0):e.v=t,e.deps===null))){Ee(e,Re);return}Ht||(Fe!==null?(ko()||U!=null&&U.is_fork)&&Fe.set(e,t):bo(e))}function oi(e){var t,r;if(e.effects!==null)for(const a of e.effects)(a.teardown||a.ac)&&((t=a.teardown)==null||t.call(a),(r=a.ac)==null||r.abort(Rt),a.teardown=gr,a.ac=null,ta(a,0),$o(a))}function mn(e){if(e.effects!==null)for(const t of e.effects)t.teardown&&Br(t)}let ao=new Set;const yr=new Map;let gn=!1;function or(e,t){var r={f:0,v:e,reactions:null,equals:nn,rv:0,wv:0};return r}function W(e,t){const r=or(e);return Rn(r),r}function ni(e,t=!1,r=!0){var o;const a=or(e);return t||(a.equals=sn),da&&r&&ze!==null&&ze.l!==null&&((o=ze.l).s??(o.s=[])).push(a),a}function S(e,t,r=!1){ne!==null&&(!$t||(ne.f&Ya)!==0)&&ua()&&(ne.f&(Ve|Pt|ea|Ya))!==0&&(ht===null||!Rr.call(ht,e))&&Os();let a=r?me(t):t;return jr(e,a,ya)}function jr(e,t,r=null){if(!e.equals(t)){yr.set(e,Ht?t:e.v);var a=Sr.ensure();if(a.capture(e,t),(e.f&Ve)!==0){const o=e;(e.f&Be)!==0&&wo(o),Fe===null&&bo(o)}e.wv=zn(),yn(e,Be,r),ua()&&te!==null&&(te.f&Re)!==0&&(te.f&(St|rr))===0&&(dt===null?mi([e]):dt.push(e)),!a.is_fork&&ao.size>0&&!gn&&si()}return t}function si(){gn=!1;for(const e of ao)(e.f&Re)!==0&&Ee(e,Mt),ha(e)&&Br(e);ao.clear()}function Oo(e,t=1){var r=n(e),a=t===1?r++:r--;return S(e,r),a}function Xr(e){S(e,e.v+1)}function yn(e,t,r){var a=e.reactions;if(a!==null)for(var o=ua(),s=a.length,i=0;i<s;i++){var l=a[i],c=l.f;if(!(!o&&l===te)){var d=(c&Be)===0;if(d&&Ee(l,t),(c&Ve)!==0){var f=l;Fe==null||Fe.delete(f),(c&$r)===0&&(c&pt&&(l.f|=$r),yn(f,Mt,r))}else if(d){var g=l;(c&Pt)!==0&&wt!==null&&wt.add(g),r!==null?r.push(g):go(g)}}}}function me(e){if(typeof e!="object"||e===null||Bt in e)return e;const t=po(e);if(t!==ms&&t!==gs)return e;var r=new Map,a=vo(e),o=W(0),s=kr,i=l=>{if(kr===s)return l();var c=ne,d=kr;_t(null),zo(s);var f=l();return _t(c),zo(d),f};return a&&r.set("length",W(e.length)),new Proxy(e,{defineProperty(l,c,d){(!("value"in d)||d.configurable===!1||d.enumerable===!1||d.writable===!1)&&Ps();var f=r.get(c);return f===void 0?i(()=>{var g=W(d.value);return r.set(c,g),g}):S(f,d.value,!0),!0},deleteProperty(l,c){var d=r.get(c);if(d===void 0){if(c in l){const f=i(()=>W(Le));r.set(c,f),Xr(o)}}else S(d,Le),Xr(o);return!0},get(l,c,d){var y;if(c===Bt)return e;var f=r.get(c),g=c in l;if(f===void 0&&(!g||(y=er(l,c))!=null&&y.writable)&&(f=i(()=>{var p=me(g?l[c]:Le),$=W(p);return $}),r.set(c,f)),f!==void 0){var b=n(f);return b===Le?void 0:b}return Reflect.get(l,c,d)},getOwnPropertyDescriptor(l,c){var d=Reflect.getOwnPropertyDescriptor(l,c);if(d&&"value"in d){var f=r.get(c);f&&(d.value=n(f))}else if(d===void 0){var g=r.get(c),b=g==null?void 0:g.v;if(g!==void 0&&b!==Le)return{enumerable:!0,configurable:!0,value:b,writable:!0}}return d},has(l,c){var b;if(c===Bt)return!0;var d=r.get(c),f=d!==void 0&&d.v!==Le||Reflect.has(l,c);if(d!==void 0||te!==null&&(!f||(b=er(l,c))!=null&&b.writable)){d===void 0&&(d=i(()=>{var y=f?me(l[c]):Le,p=W(y);return p}),r.set(c,d));var g=n(d);if(g===Le)return!1}return f},set(l,c,d,f){var C;var g=r.get(c),b=c in l;if(a&&c==="length")for(var y=d;y<g.v;y+=1){var p=r.get(y+"");p!==void 0?S(p,Le):y in l&&(p=i(()=>W(Le)),r.set(y+"",p))}if(g===void 0)(!b||(C=er(l,c))!=null&&C.writable)&&(g=i(()=>W(void 0)),S(g,me(d)),r.set(c,g));else{b=g.v!==Le;var $=i(()=>me(d));S(g,$)}var x=Reflect.getOwnPropertyDescriptor(l,c);if(x!=null&&x.set&&x.set.call(f,d),!b){if(a&&typeof c=="string"){var v=r.get("length"),A=Number(c);Number.isInteger(A)&&A>=v.v&&S(v,A+1)}Xr(o)}return!0},ownKeys(l){n(o);var c=Reflect.ownKeys(l).filter(g=>{var b=r.get(g);return b===void 0||b.v!==Le});for(var[d,f]of r)f.v!==Le&&!(d in l)&&c.push(d);return c},setPrototypeOf(){Ms()}})}function Co(e){try{if(e!==null&&typeof e=="object"&&Bt in e)return e[Bt]}catch{}return e}function ii(e,t){return Object.is(Co(e),Co(t))}var Lo,wn,kn,$n;function li(){if(Lo===void 0){Lo=window,wn=/Firefox/.test(navigator.userAgent);var e=Element.prototype,t=Node.prototype,r=Text.prototype;kn=er(t,"firstChild").get,$n=er(t,"nextSibling").get,To(e)&&(e.__click=void 0,e.__className=void 0,e.__attributes=null,e.__style=void 0,e.__e=void 0),To(r)&&(r.__t=void 0)}}function Vt(e=""){return document.createTextNode(e)}function Fr(e){return kn.call(e)}function va(e){return $n.call(e)}function u(e,t){return Fr(e)}function ie(e,t=!1){{var r=Fr(e);return r instanceof Comment&&r.data===""?va(r):r}}function _(e,t=1,r=!1){let a=e;for(;t--;)a=va(a);return a}function ci(e){e.textContent=""}function Sn(){return!1}function En(e,t,r){return document.createElementNS(t??en,e,void 0)}function di(e,t){if(t){const r=document.body;e.autofocus=!0,Wt(()=>{document.activeElement===r&&e.focus()})}}let Ro=!1;function ui(){Ro||(Ro=!0,document.addEventListener("reset",e=>{Promise.resolve().then(()=>{var t;if(!e.defaultPrevented)for(const r of e.target.elements)(t=r.__on_r)==null||t.call(r)})},{capture:!0}))}function Ca(e){var t=ne,r=te;_t(null),xt(null);try{return e()}finally{_t(t),xt(r)}}function fi(e,t,r,a=r){e.addEventListener(t,()=>Ca(r));const o=e.__on_r;o?e.__on_r=()=>{o(),a(!0)}:e.__on_r=()=>a(!0),ui()}function An(e){te===null&&(ne===null&&Ns(),As()),Ht&&Es()}function vi(e,t){var r=t.last;r===null?t.last=t.first=e:(r.next=e,e.prev=r,t.last=e)}function Et(e,t){var r=te;r!==null&&(r.f&qe)!==0&&(e|=qe);var a={ctx:ze,deps:null,nodes:null,f:e|Be|pt,first:null,fn:t,last:null,next:null,parent:r,b:r&&r.b,prev:null,teardown:null,wv:0,ac:null};U==null||U.register_created_effect(a);var o=a;if((e&Dr)!==0)Ir!==null?Ir.push(a):Sr.ensure().schedule(a);else if(t!==null){try{Br(a)}catch(i){throw We(a),i}o.deps===null&&o.teardown===null&&o.nodes===null&&o.first===o.last&&(o.f&Vr)===0&&(o=o.first,(e&Pt)!==0&&(e&ar)!==0&&o!==null&&(o.f|=ar))}if(o!==null&&(o.parent=r,r!==null&&vi(o,r),ne!==null&&(ne.f&Ve)!==0&&(e&rr)===0)){var s=ne;(s.effects??(s.effects=[])).push(o)}return a}function ko(){return ne!==null&&!$t}function La(e){const t=Et(ca,null);return Ee(t,Re),t.teardown=e,t}function Ke(e){An();var t=te.f,r=!ne&&(t&St)!==0&&(t&Er)===0;if(r){var a=ze;(a.e??(a.e=[])).push(e)}else return Nn(e)}function Nn(e){return Et(Dr|an,e)}function pi(e){return An(),Et(ca|an,e)}function hi(e){Sr.ensure();const t=Et(rr|Vr,e);return(r={})=>new Promise(a=>{r.outro?wr(t,()=>{We(t),a(void 0)}):(We(t),a(void 0))})}function Tn(e){return Et(Dr,e)}function _i(e){return Et(ea|Vr,e)}function In(e,t=0){return Et(ca|t,e)}function F(e,t=[],r=[],a=[]){_n(a,t,r,o=>{Et(ca,()=>e(...o.map(n)))})}function pa(e,t=0){var r=Et(Pt|t,e);return r}function Pn(e,t=0){var r=Et(_o|t,e);return r}function tt(e){return Et(St|Vr,e)}function Mn(e){var t=e.teardown;if(t!==null){const r=Ht,a=ne;Do(!0),_t(null);try{t.call(null)}finally{Do(r),_t(a)}}}function $o(e,t=!1){var r=e.first;for(e.first=e.last=null;r!==null;){const o=r.ac;o!==null&&Ca(()=>{o.abort(Rt)});var a=r.next;(r.f&rr)!==0?r.parent=null:We(r,t),r=a}}function xi(e){for(var t=e.first;t!==null;){var r=t.next;(t.f&St)===0&&We(t),t=r}}function We(e,t=!0){var r=!1;(t||(e.f&ws)!==0)&&e.nodes!==null&&e.nodes.end!==null&&(bi(e.nodes.start,e.nodes.end),r=!0),Ee(e,Io),$o(e,t&&!r),ta(e,0);var a=e.nodes&&e.nodes.t;if(a!==null)for(const s of a)s.stop();Mn(e),e.f^=Io,e.f|=lt;var o=e.parent;o!==null&&o.first!==null&&On(e),e.next=e.prev=e.teardown=e.ctx=e.deps=e.fn=e.nodes=e.ac=e.b=null}function bi(e,t){for(;e!==null;){var r=e===t?null:va(e);e.remove(),e=r}}function On(e){var t=e.parent,r=e.prev,a=e.next;r!==null&&(r.next=a),a!==null&&(a.prev=r),t!==null&&(t.first===e&&(t.first=a),t.last===e&&(t.last=r))}function wr(e,t,r=!0){var a=[];Cn(e,a,!0);var o=()=>{r&&We(e),t&&t()},s=a.length;if(s>0){var i=()=>--s||o();for(var l of a)l.out(i)}else o()}function Cn(e,t,r){if((e.f&qe)===0){e.f^=qe;var a=e.nodes&&e.nodes.t;if(a!==null)for(const l of a)(l.is_global||r)&&t.push(l);for(var o=e.first;o!==null;){var s=o.next;if((o.f&rr)===0){var i=(o.f&ar)!==0||(o.f&St)!==0&&(e.f&Pt)!==0;Cn(o,t,i?r:!1)}o=s}}}function So(e){Ln(e,!0)}function Ln(e,t){if((e.f&qe)!==0){e.f^=qe,(e.f&Re)===0&&(Ee(e,Be),Sr.ensure().schedule(e));for(var r=e.first;r!==null;){var a=r.next,o=(r.f&ar)!==0||(r.f&St)!==0;Ln(r,o?t:!1),r=a}var s=e.nodes&&e.nodes.t;if(s!==null)for(const i of s)(i.is_global||t)&&i.in()}}function Eo(e,t){if(e.nodes)for(var r=e.nodes.start,a=e.nodes.end;r!==null;){var o=r===a?null:va(r);t.append(r),r=o}}let Aa=!1,Ht=!1;function Do(e){Ht=e}let ne=null,$t=!1;function _t(e){ne=e}let te=null;function xt(e){te=e}let ht=null;function Rn(e){ne!==null&&(ht===null?ht=[e]:ht.push(e))}let et=null,ot=0,dt=null;function mi(e){dt=e}let Dn=1,vr=0,kr=vr;function zo(e){kr=e}function zn(){return++Dn}function ha(e){var t=e.f;if((t&Be)!==0)return!0;if(t&Ve&&(e.f&=~$r),(t&Mt)!==0){for(var r=e.deps,a=r.length,o=0;o<a;o++){var s=r[o];if(ha(s)&&bn(s),s.wv>e.wv)return!0}(t&pt)!==0&&Fe===null&&Ee(e,Re)}return!1}function jn(e,t,r=!0){var a=e.reactions;if(a!==null&&!(ht!==null&&Rr.call(ht,e)))for(var o=0;o<a.length;o++){var s=a[o];(s.f&Ve)!==0?jn(s,t,!1):t===s&&(r?Ee(s,Be):(s.f&Re)!==0&&Ee(s,Mt),go(s))}}function Fn(e){var $;var t=et,r=ot,a=dt,o=ne,s=ht,i=ze,l=$t,c=kr,d=e.f;et=null,ot=0,dt=null,ne=(d&(St|rr))===0?e:null,ht=null,zr(e.ctx),$t=!1,kr=++vr,e.ac!==null&&(Ca(()=>{e.ac.abort(Rt)}),e.ac=null);try{e.f|=Za;var f=e.fn,g=f();e.f|=Er;var b=e.deps,y=U==null?void 0:U.is_fork;if(et!==null){var p;if(y||ta(e,ot),b!==null&&ot>0)for(b.length=ot+et.length,p=0;p<et.length;p++)b[ot+p]=et[p];else e.deps=b=et;if(ko()&&(e.f&pt)!==0)for(p=ot;p<b.length;p++)(($=b[p]).reactions??($.reactions=[])).push(e)}else!y&&b!==null&&ot<b.length&&(ta(e,ot),b.length=ot);if(ua()&&dt!==null&&!$t&&b!==null&&(e.f&(Ve|Mt|Be))===0)for(p=0;p<dt.length;p++)jn(dt[p],e);if(o!==null&&o!==e){if(vr++,o.deps!==null)for(let x=0;x<r;x+=1)o.deps[x].rv=vr;if(t!==null)for(const x of t)x.rv=vr;dt!==null&&(a===null?a=dt:a.push(...dt))}return(e.f&tr)!==0&&(e.f^=tr),g}catch(x){return cn(x)}finally{e.f^=Za,et=t,ot=r,dt=a,ne=o,ht=s,zr(i),$t=l,kr=c}}function gi(e,t){let r=t.reactions;if(r!==null){var a=xs.call(r,e);if(a!==-1){var o=r.length-1;o===0?r=t.reactions=null:(r[a]=r[o],r.pop())}}if(r===null&&(t.f&Ve)!==0&&(et===null||!Rr.call(et,t))){var s=t;(s.f&pt)!==0&&(s.f^=pt,s.f&=~$r),s.v!==Le&&bo(s),oi(s),ta(s,0)}}function ta(e,t){var r=e.deps;if(r!==null)for(var a=t;a<r.length;a++)gi(e,r[a])}function Br(e){var t=e.f;if((t&lt)===0){Ee(e,Re);var r=te,a=Aa;te=e,Aa=!0;try{(t&(Pt|_o))!==0?xi(e):$o(e),Mn(e);var o=Fn(e);e.teardown=typeof o=="function"?o:null,e.wv=Dn;var s;_s&&js&&(e.f&Be)!==0&&e.deps}finally{Aa=a,te=r}}}async function yi(){await Promise.resolve(),qs()}function n(e){var t=e.f,r=(t&Ve)!==0;if(ne!==null&&!$t){var a=te!==null&&(te.f&lt)!==0;if(!a&&(ht===null||!Rr.call(ht,e))){var o=ne.deps;if((ne.f&Za)!==0)e.rv<vr&&(e.rv=vr,et===null&&o!==null&&o[ot]===e?ot++:et===null?et=[e]:et.push(e));else{(ne.deps??(ne.deps=[])).push(e);var s=e.reactions;s===null?e.reactions=[ne]:Rr.call(s,ne)||s.push(ne)}}}if(Ht&&yr.has(e))return yr.get(e);if(r){var i=e;if(Ht){var l=i.v;return((i.f&Re)===0&&i.reactions!==null||Wn(i))&&(l=wo(i)),yr.set(i,l),l}var c=(i.f&pt)===0&&!$t&&ne!==null&&(Aa||(ne.f&pt)!==0),d=(i.f&Er)===0;ha(i)&&(c&&(i.f|=pt),bn(i)),c&&!d&&(mn(i),Bn(i))}if(Fe!=null&&Fe.has(e))return Fe.get(e);if((e.f&tr)!==0)throw e.v;return e.v}function Bn(e){if(e.f|=pt,e.deps!==null)for(const t of e.deps)(t.reactions??(t.reactions=[])).push(e),(t.f&Ve)!==0&&(t.f&pt)===0&&(mn(t),Bn(t))}function Wn(e){if(e.v===Le)return!0;if(e.deps===null)return!1;for(const t of e.deps)if(yr.has(t)||(t.f&Ve)!==0&&Wn(t))return!0;return!1}function Wr(e){var t=$t;try{return $t=!0,e()}finally{$t=t}}function ur(e){if(!(typeof e!="object"||!e||e instanceof EventTarget)){if(Bt in e)oo(e);else if(!Array.isArray(e))for(let t in e){const r=e[t];typeof r=="object"&&r&&Bt in r&&oo(r)}}}function oo(e,t=new Set){if(typeof e=="object"&&e!==null&&!(e instanceof EventTarget)&&!t.has(e)){t.add(e),e instanceof Date&&e.getTime();for(let a in e)try{oo(e[a],t)}catch{}const r=po(e);if(r!==Object.prototype&&r!==Array.prototype&&r!==Map.prototype&&r!==Set.prototype&&r!==Date.prototype){const a=tn(r);for(let o in a){const s=a[o].get;if(s)try{s.call(e)}catch{}}}}}const pr=Symbol("events"),Vn=new Set,no=new Set;function Hn(e,t,r,a={}){function o(s){if(a.capture||so.call(t,s),!s.cancelBubble)return Ca(()=>r==null?void 0:r.call(this,s))}return e.startsWith("pointer")||e.startsWith("touch")||e==="wheel"?Wt(()=>{t.addEventListener(e,o,a)}):t.addEventListener(e,o,a),o}function Un(e,t,r,a,o){var s={capture:a,passive:o},i=Hn(e,t,r,s);(t===document.body||t===window||t===document||t instanceof HTMLMediaElement)&&La(()=>{t.removeEventListener(e,i,s)})}function Z(e,t,r){(t[pr]??(t[pr]={}))[e]=r}function Ut(e){for(var t=0;t<e.length;t++)Vn.add(e[t]);for(var r of no)r(e)}let jo=null;function so(e){var x,v;var t=this,r=t.ownerDocument,a=e.type,o=((x=e.composedPath)==null?void 0:x.call(e))||[],s=o[0]||e.target;jo=e;var i=0,l=jo===e&&e[pr];if(l){var c=o.indexOf(l);if(c!==-1&&(t===document||t===window)){e[pr]=t;return}var d=o.indexOf(t);if(d===-1)return;c<=d&&(i=c)}if(s=o[i]||e.target,s!==t){bs(e,"currentTarget",{configurable:!0,get(){return s||r}});var f=ne,g=te;_t(null),xt(null);try{for(var b,y=[];s!==null;){var p=s.assignedSlot||s.parentNode||s.host||null;try{var $=(v=s[pr])==null?void 0:v[a];$!=null&&(!s.disabled||e.target===s)&&$.call(s,e)}catch(A){b?y.push(A):b=A}if(e.cancelBubble||p===t||p===null)break;s=p}if(b){for(let A of y)queueMicrotask(()=>{throw A});throw b}}finally{e[pr]=t,delete e.currentTarget,_t(f),xt(g)}}}var Zo;const Wa=((Zo=globalThis==null?void 0:globalThis.window)==null?void 0:Zo.trustedTypes)&&globalThis.window.trustedTypes.createPolicy("svelte-trusted-html",{createHTML:e=>e});function wi(e){return(Wa==null?void 0:Wa.createHTML(e))??e}function qn(e){var t=En("template");return t.innerHTML=wi(e.replaceAll("<!>","<!---->")),t.content}function ra(e,t){var r=te;r.nodes===null&&(r.nodes={start:e,end:t,a:null,t:null})}function w(e,t){var r=(t&fs)!==0,a=(t&vs)!==0,o,s=!e.startsWith("<!>");return()=>{o===void 0&&(o=qn(s?e:"<!>"+e),r||(o=Fr(o)));var i=a||wn?document.importNode(o,!0):o.cloneNode(!0);if(r){var l=Fr(i),c=i.lastChild;ra(l,c)}else ra(i,i);return i}}function ki(e,t,r="svg"){var a=!e.startsWith("<!>"),o=`<${r}>${a?e:"<!>"+e}</${r}>`,s;return()=>{if(!s){var i=qn(o),l=Fr(i);s=Fr(l)}var c=s.cloneNode(!0);return ra(c,c),c}}function $i(e,t){return ki(e,t,"svg")}function ue(){var e=document.createDocumentFragment(),t=document.createComment(""),r=Vt();return e.append(t,r),ra(t,r),e}function m(e,t){e!==null&&e.before(t)}function Si(e){return e.endsWith("capture")&&e!=="gotpointercapture"&&e!=="lostpointercapture"}const Ei=["beforeinput","click","change","dblclick","contextmenu","focusin","focusout","input","keydown","keyup","mousedown","mousemove","mouseout","mouseover","mouseup","pointerdown","pointermove","pointerout","pointerover","pointerup","touchend","touchmove","touchstart"];function Ai(e){return Ei.includes(e)}const Ni={formnovalidate:"formNoValidate",ismap:"isMap",nomodule:"noModule",playsinline:"playsInline",readonly:"readOnly",defaultvalue:"defaultValue",defaultchecked:"defaultChecked",srcobject:"srcObject",novalidate:"noValidate",allowfullscreen:"allowFullscreen",disablepictureinpicture:"disablePictureInPicture",disableremoteplayback:"disableRemotePlayback"};function Ti(e){return e=e.toLowerCase(),Ni[e]??e}const Ii=["touchstart","touchmove"];function Pi(e){return Ii.includes(e)}function I(e,t){var r=t==null?"":typeof t=="object"?`${t}`:t;r!==(e.__t??(e.__t=e.nodeValue))&&(e.__t=r,e.nodeValue=`${r}`)}function Mi(e,t){return Oi(e,t)}const ga=new Map;function Oi(e,{target:t,anchor:r,props:a={},events:o,context:s,intro:i=!0,transformError:l}){li();var c=void 0,d=hi(()=>{var f=r??t.appendChild(Vt());Ys(f,{pending:()=>{}},y=>{Je({});var p=ze;s&&(p.c=s),o&&(a.$$events=o),c=e(y,a)||{},Ge()},l);var g=new Set,b=y=>{for(var p=0;p<y.length;p++){var $=y[p];if(!g.has($)){g.add($);var x=Pi($);for(const C of[t,document]){var v=ga.get(C);v===void 0&&(v=new Map,ga.set(C,v));var A=v.get($);A===void 0?(C.addEventListener($,so,{passive:x}),v.set($,1)):v.set($,A+1)}}}};return b(Oa(Vn)),no.add(b),()=>{var x;for(var y of g)for(const v of[t,document]){var p=ga.get(v),$=p.get(y);--$==0?(v.removeEventListener(y,so),p.delete(y),p.size===0&&ga.delete(v)):p.set(y,$)}no.delete(b),f!==r&&((x=f.parentNode)==null||x.removeChild(f))}});return Ci.set(c,d),c}let Ci=new WeakMap;var kt,Tt,it,mr,ia,la,Ma;class Ao{constructor(t,r=!0){yt(this,"anchor");X(this,kt,new Map);X(this,Tt,new Map);X(this,it,new Map);X(this,mr,new Set);X(this,ia,!0);X(this,la,t=>{if(h(this,kt).has(t)){var r=h(this,kt).get(t),a=h(this,Tt).get(r);if(a)So(a),h(this,mr).delete(r);else{var o=h(this,it).get(r);o&&(h(this,Tt).set(r,o.effect),h(this,it).delete(r),o.fragment.lastChild.remove(),this.anchor.before(o.fragment),a=o.effect)}for(const[s,i]of h(this,kt)){if(h(this,kt).delete(s),s===t)break;const l=h(this,it).get(i);l&&(We(l.effect),h(this,it).delete(i))}for(const[s,i]of h(this,Tt)){if(s===r||h(this,mr).has(s))continue;const l=()=>{if(Array.from(h(this,kt).values()).includes(s)){var d=document.createDocumentFragment();Eo(i,d),d.append(Vt()),h(this,it).set(s,{effect:i,fragment:d})}else We(i);h(this,mr).delete(s),h(this,Tt).delete(s)};h(this,ia)||!a?(h(this,mr).add(s),wr(i,l,!1)):l()}}});X(this,Ma,t=>{h(this,kt).delete(t);const r=Array.from(h(this,kt).values());for(const[a,o]of h(this,it))r.includes(a)||(We(o.effect),h(this,it).delete(a))});this.anchor=t,re(this,ia,r)}ensure(t,r){var a=U,o=Sn();if(r&&!h(this,Tt).has(t)&&!h(this,it).has(t))if(o){var s=document.createDocumentFragment(),i=Vt();s.append(i),h(this,it).set(t,{effect:tt(()=>r(i)),fragment:s})}else h(this,Tt).set(t,tt(()=>r(this.anchor)));if(h(this,kt).set(a,t),o){for(const[l,c]of h(this,Tt))l===t?a.unskip_effect(c):a.skip_effect(c);for(const[l,c]of h(this,it))l===t?a.unskip_effect(c.effect):a.skip_effect(c.effect);a.oncommit(h(this,la)),a.ondiscard(h(this,Ma))}else h(this,la).call(this,a)}}kt=new WeakMap,Tt=new WeakMap,it=new WeakMap,mr=new WeakMap,ia=new WeakMap,la=new WeakMap,Ma=new WeakMap;function J(e,t,r=!1){var a=new Ao(e),o=r?ar:0;function s(i,l){a.ensure(i,l)}pa(()=>{var i=!1;t((l,c=0)=>{i=!0,s(c,l)}),i||s(-1,null)},o)}function $e(e,t){return t}function Li(e,t,r){for(var a=[],o=t.length,s,i=t.length,l=0;l<o;l++){let g=t[l];wr(g,()=>{if(s){if(s.pending.delete(g),s.done.add(g),s.pending.size===0){var b=e.outrogroups;io(e,Oa(s.done)),b.delete(s),b.size===0&&(e.outrogroups=null)}}else i-=1},!1)}if(i===0){var c=a.length===0&&r!==null;if(c){var d=r,f=d.parentNode;ci(f),f.append(d),e.items.clear()}io(e,t,!c)}else s={pending:new Set(t),done:new Set},(e.outrogroups??(e.outrogroups=new Set)).add(s)}function io(e,t,r=!0){var a;if(e.pending.size>0){a=new Set;for(const i of e.pending.values())for(const l of i)a.add(e.items.get(l).e)}for(var o=0;o<t.length;o++){var s=t[o];if(a!=null&&a.has(s)){s.f|=It;const i=document.createDocumentFragment();Eo(s,i)}else We(t[o],r)}}var Fo;function Se(e,t,r,a,o,s=null){var i=e,l=new Map,c=(t&Xo)!==0;if(c){var d=e;i=d.appendChild(Vt())}var f=null,g=yo(()=>{var C=r();return vo(C)?C:C==null?[]:Oa(C)}),b,y=new Map,p=!0;function $(C){(A.effect.f&lt)===0&&(A.pending.delete(C),A.fallback=f,Ri(A,b,i,t,a),f!==null&&(b.length===0?(f.f&It)===0?So(f):(f.f^=It,Yr(f,null,i)):wr(f,()=>{f=null})))}function x(C){A.pending.delete(C)}var v=pa(()=>{b=n(g);for(var C=b.length,E=new Set,O=U,D=Sn(),T=0;T<C;T+=1){var z=b[T],P=a(z,T),H=p?null:l.get(P);H?(H.v&&jr(H.v,z),H.i&&jr(H.i,T),D&&O.unskip_effect(H.e)):(H=Di(l,p?i:Fo??(Fo=Vt()),z,P,T,o,t,r),p||(H.e.f|=It),l.set(P,H)),E.add(P)}if(C===0&&s&&!f&&(p?f=tt(()=>s(i)):(f=tt(()=>s(Fo??(Fo=Vt()))),f.f|=It)),C>E.size&&Ss(),!p)if(y.set(O,E),D){for(const[B,K]of l)E.has(B)||O.skip_effect(K.e);O.oncommit($),O.ondiscard(x)}else $(O);n(g)}),A={effect:v,items:l,pending:y,outrogroups:null,fallback:f};p=!1}function Kr(e){for(;e!==null&&(e.f&St)===0;)e=e.next;return e}function Ri(e,t,r,a,o){var H,B,K,R,j,V,G,le,M;var s=(a&ss)!==0,i=t.length,l=e.items,c=Kr(e.effect.first),d,f=null,g,b=[],y=[],p,$,x,v;if(s)for(v=0;v<i;v+=1)p=t[v],$=o(p,v),x=l.get($).e,(x.f&It)===0&&((B=(H=x.nodes)==null?void 0:H.a)==null||B.measure(),(g??(g=new Set)).add(x));for(v=0;v<i;v+=1){if(p=t[v],$=o(p,v),x=l.get($).e,e.outrogroups!==null)for(const N of e.outrogroups)N.pending.delete(x),N.done.delete(x);if((x.f&qe)!==0&&(So(x),s&&((R=(K=x.nodes)==null?void 0:K.a)==null||R.unfix(),(g??(g=new Set)).delete(x))),(x.f&It)!==0)if(x.f^=It,x===c)Yr(x,null,r);else{var A=f?f.next:c;x===e.effect.last&&(e.effect.last=x.prev),x.prev&&(x.prev.next=x.next),x.next&&(x.next.prev=x.prev),Gt(e,f,x),Gt(e,x,A),Yr(x,A,r),f=x,b=[],y=[],c=Kr(f.next);continue}if(x!==c){if(d!==void 0&&d.has(x)){if(b.length<y.length){var C=y[0],E;f=C.prev;var O=b[0],D=b[b.length-1];for(E=0;E<b.length;E+=1)Yr(b[E],C,r);for(E=0;E<y.length;E+=1)d.delete(y[E]);Gt(e,O.prev,D.next),Gt(e,f,O),Gt(e,D,C),c=C,f=D,v-=1,b=[],y=[]}else d.delete(x),Yr(x,c,r),Gt(e,x.prev,x.next),Gt(e,x,f===null?e.effect.first:f.next),Gt(e,f,x),f=x;continue}for(b=[],y=[];c!==null&&c!==x;)(d??(d=new Set)).add(c),y.push(c),c=Kr(c.next);if(c===null)continue}(x.f&It)===0&&b.push(x),f=x,c=Kr(x.next)}if(e.outrogroups!==null){for(const N of e.outrogroups)N.pending.size===0&&(io(e,Oa(N.done)),(j=e.outrogroups)==null||j.delete(N));e.outrogroups.size===0&&(e.outrogroups=null)}if(c!==null||d!==void 0){var T=[];if(d!==void 0)for(x of d)(x.f&qe)===0&&T.push(x);for(;c!==null;)(c.f&qe)===0&&c!==e.fallback&&T.push(c),c=Kr(c.next);var z=T.length;if(z>0){var P=(a&Xo)!==0&&i===0?r:null;if(s){for(v=0;v<z;v+=1)(G=(V=T[v].nodes)==null?void 0:V.a)==null||G.measure();for(v=0;v<z;v+=1)(M=(le=T[v].nodes)==null?void 0:le.a)==null||M.fix()}Li(e,T,P)}}s&&Wt(()=>{var N,Y;if(g!==void 0)for(x of g)(Y=(N=x.nodes)==null?void 0:N.a)==null||Y.apply()})}function Di(e,t,r,a,o,s,i,l){var c=(i&os)!==0?(i&is)===0?ni(r,!1,!1):or(r):null,d=(i&ns)!==0?or(o):null;return{v:c,i:d,e:tt(()=>(s(t,c??r,d??o,l),()=>{e.delete(a)}))}}function Yr(e,t,r){if(e.nodes)for(var a=e.nodes.start,o=e.nodes.end,s=t&&(t.f&It)===0?t.nodes.start:r;a!==null;){var i=va(a);if(s.before(a),a===o)return;a=i}}function Gt(e,t,r){t===null?e.effect.first=r:t.next=r,r===null?e.effect.last=t:r.prev=t}function ke(e,t,r,a,o){var l;var s=(l=t.$$slots)==null?void 0:l[r],i=!1;s===!0&&(s=t.children,i=!0),s===void 0||s(e,i?()=>a:a)}function zi(e,t,r){var a=new Ao(e);pa(()=>{var o=t()??null;a.ensure(o,o&&(s=>r(s,o)))},ar)}function ji(e,t,r,a,o,s){var i=null,l=e,c=new Ao(l,!1);pa(()=>{const d=t()||null;var f=ps;if(d===null){c.ensure(null,null);return}return c.ensure(d,g=>{if(d){if(i=En(d,f),ra(i,i),a){var b=i.appendChild(Vt());a(i,b)}te.nodes.end=i,g.before(i)}}),()=>{}},ar),La(()=>{})}function Fi(e,t){var r=void 0,a;Pn(()=>{r!==(r=t())&&(a&&(We(a),a=null),r&&(a=tt(()=>{Tn(()=>r(e))})))})}function Kn(e){var t,r,a="";if(typeof e=="string"||typeof e=="number")a+=e;else if(typeof e=="object")if(Array.isArray(e)){var o=e.length;for(t=0;t<o;t++)e[t]&&(r=Kn(e[t]))&&(a&&(a+=" "),a+=r)}else for(r in e)e[r]&&(a&&(a+=" "),a+=r);return a}function Bi(){for(var e,t,r=0,a="",o=arguments.length;r<o;r++)(e=arguments[r])&&(t=Kn(e))&&(a&&(a+=" "),a+=t);return a}function Wi(e){return typeof e=="object"?Bi(e):e??""}const Bo=[...` 	
\r\f \v\uFEFF`];function Vi(e,t,r){var a=e==null?"":""+e;if(r){for(var o of Object.keys(r))if(r[o])a=a?a+" "+o:o;else if(a.length)for(var s=o.length,i=0;(i=a.indexOf(o,i))>=0;){var l=i+s;(i===0||Bo.includes(a[i-1]))&&(l===a.length||Bo.includes(a[l]))?a=(i===0?"":a.substring(0,i))+a.substring(l+1):i=l}}return a===""?null:a}function Wo(e,t=!1){var r=t?" !important;":";",a="";for(var o of Object.keys(e)){var s=e[o];s!=null&&s!==""&&(a+=" "+o+": "+s+r)}return a}function Va(e){return e[0]!=="-"||e[1]!=="-"?e.toLowerCase():e}function Hi(e,t){if(t){var r="",a,o;if(Array.isArray(t)?(a=t[0],o=t[1]):a=t,e){e=String(e).replaceAll(/\s*\/\*.*?\*\/\s*/g,"").trim();var s=!1,i=0,l=!1,c=[];a&&c.push(...Object.keys(a).map(Va)),o&&c.push(...Object.keys(o).map(Va));var d=0,f=-1;const $=e.length;for(var g=0;g<$;g++){var b=e[g];if(l?b==="/"&&e[g-1]==="*"&&(l=!1):s?s===b&&(s=!1):b==="/"&&e[g+1]==="*"?l=!0:b==='"'||b==="'"?s=b:b==="("?i++:b===")"&&i--,!l&&s===!1&&i===0){if(b===":"&&f===-1)f=g;else if(b===";"||g===$-1){if(f!==-1){var y=Va(e.substring(d,f).trim());if(!c.includes(y)){b!==";"&&g++;var p=e.substring(d,g).trim();r+=" "+p+";"}}d=g+1,f=-1}}}}return a&&(r+=Wo(a)),o&&(r+=Wo(o,!0)),r=r.trim(),r===""?null:r}return e==null?null:String(e)}function Ie(e,t,r,a,o,s){var i=e.__className;if(i!==r||i===void 0){var l=Vi(r,a,s);l==null?e.removeAttribute("class"):t?e.className=l:e.setAttribute("class",l),e.__className=r}else if(s&&o!==s)for(var c in s){var d=!!s[c];(o==null||d!==!!o[c])&&e.classList.toggle(c,d)}return s}function Ha(e,t={},r,a){for(var o in r){var s=r[o];t[o]!==s&&(r[o]==null?e.style.removeProperty(o):e.style.setProperty(o,s,a))}}function Ui(e,t,r,a){var o=e.__style;if(o!==t){var s=Hi(t,a);s==null?e.removeAttribute("style"):e.style.cssText=s,e.__style=t}else a&&(Array.isArray(a)?(Ha(e,r==null?void 0:r[0],a[0]),Ha(e,r==null?void 0:r[1],a[1],"important")):Ha(e,r,a));return a}function lo(e,t,r=!1){if(e.multiple){if(t==null)return;if(!vo(t))return Rs();for(var a of e.options)a.selected=t.includes(Vo(a));return}for(a of e.options){var o=Vo(a);if(ii(o,t)){a.selected=!0;return}}(!r||t!==void 0)&&(e.selectedIndex=-1)}function qi(e){var t=new MutationObserver(()=>{lo(e,e.__value)});t.observe(e,{childList:!0,subtree:!0,attributes:!0,attributeFilter:["value"]}),La(()=>{t.disconnect()})}function Vo(e){return"__value"in e?e.__value:e.value}const Jr=Symbol("class"),Gr=Symbol("style"),Jn=Symbol("is custom element"),Gn=Symbol("is html"),Ki=xo?"option":"OPTION",Ji=xo?"select":"SELECT",Gi=xo?"progress":"PROGRESS";function dr(e,t){var r=Ra(e);r.value===(r.value=t??void 0)||e.value===t&&(t!==0||e.nodeName!==Gi)||(e.value=t??"")}function Ua(e,t){var r=Ra(e);r.checked!==(r.checked=t??void 0)&&(e.checked=t)}function Yi(e,t){t?e.hasAttribute("selected")||e.setAttribute("selected",""):e.removeAttribute("selected")}function Ta(e,t,r,a){var o=Ra(e);o[t]!==(o[t]=r)&&(t==="loading"&&(e[ks]=r),r==null?e.removeAttribute(t):typeof r!="string"&&Yn(e).includes(t)?e[t]=r:e.setAttribute(t,r))}function Zi(e,t,r,a,o=!1,s=!1){var i=Ra(e),l=i[Jn],c=!i[Gn],d=t||{},f=e.nodeName===Ki;for(var g in t)g in r||(r[g]=null);r.class?r.class=Wi(r.class):r[Jr]&&(r.class=null),r[Gr]&&(r.style??(r.style=null));var b=Yn(e);for(const E in r){let O=r[E];if(f&&E==="value"&&O==null){e.value=e.__value="",d[E]=O;continue}if(E==="class"){var y=e.namespaceURI==="http://www.w3.org/1999/xhtml";Ie(e,y,O,a,t==null?void 0:t[Jr],r[Jr]),d[E]=O,d[Jr]=r[Jr];continue}if(E==="style"){Ui(e,O,t==null?void 0:t[Gr],r[Gr]),d[E]=O,d[Gr]=r[Gr];continue}var p=d[E];if(!(O===p&&!(O===void 0&&e.hasAttribute(E)))){d[E]=O;var $=E[0]+E[1];if($!=="$$")if($==="on"){const D={},T="$$"+E;let z=E.slice(2);var x=Ai(z);if(Si(z)&&(z=z.slice(0,-7),D.capture=!0),!x&&p){if(O!=null)continue;e.removeEventListener(z,d[T],D),d[T]=null}if(x)Z(z,e,O),Ut([z]);else if(O!=null){let P=function(H){d[E].call(this,H)};var C=P;d[T]=Hn(z,e,P,D)}}else if(E==="style")Ta(e,E,O);else if(E==="autofocus")di(e,!!O);else if(!l&&(E==="__value"||E==="value"&&O!=null))e.value=e.__value=O;else if(E==="selected"&&f)Yi(e,O);else{var v=E;c||(v=Ti(v));var A=v==="defaultValue"||v==="defaultChecked";if(O==null&&!l&&!A)if(i[E]=null,v==="value"||v==="checked"){let D=e;const T=t===void 0;if(v==="value"){let z=D.defaultValue;D.removeAttribute(v),D.defaultValue=z,D.value=D.__value=T?z:null}else{let z=D.defaultChecked;D.removeAttribute(v),D.defaultChecked=z,D.checked=T?z:!1}}else e.removeAttribute(E);else A||b.includes(v)&&(l||typeof O!="string")?(e[v]=O,v in i&&(i[v]=Le)):typeof O!="function"&&Ta(e,v,O)}}}return d}function Ho(e,t,r=[],a=[],o=[],s,i=!1,l=!1){_n(o,r,a,c=>{var d=void 0,f={},g=e.nodeName===Ji,b=!1;if(Pn(()=>{var p=t(...c.map(n)),$=Zi(e,d,p,s,i,l);b&&g&&"value"in p&&lo(e,p.value);for(let v of Object.getOwnPropertySymbols(f))p[v]||We(f[v]);for(let v of Object.getOwnPropertySymbols(p)){var x=p[v];v.description===hs&&(!d||x!==d[v])&&(f[v]&&We(f[v]),f[v]=tt(()=>Fi(e,()=>x))),$[v]=x}d=$}),g){var y=e;Tn(()=>{lo(y,d.value,!0),qi(y)})}b=!0})}function Ra(e){return e.__attributes??(e.__attributes={[Jn]:e.nodeName.includes("-"),[Gn]:e.namespaceURI===en})}var Uo=new Map;function Yn(e){var t=e.getAttribute("is")||e.nodeName,r=Uo.get(t);if(r)return r;Uo.set(t,r=[]);for(var a,o=e,s=Element.prototype;s!==o;){a=tn(o);for(var i in a)a[i].set&&r.push(i);o=po(o)}return r}function nr(e,t,r=t){var a=new WeakSet;fi(e,"input",async o=>{var s=o?e.defaultValue:e.value;if(s=qa(e)?Ka(s):s,r(s),U!==null&&a.add(U),await yi(),s!==(s=t())){var i=e.selectionStart,l=e.selectionEnd,c=e.value.length;if(e.value=s??"",l!==null){var d=e.value.length;i===l&&l===c&&d>c?(e.selectionStart=d,e.selectionEnd=d):(e.selectionStart=i,e.selectionEnd=Math.min(l,d))}}}),Wr(t)==null&&e.value&&(r(qa(e)?Ka(e.value):e.value),U!==null&&a.add(U)),In(()=>{var o=t();if(e===document.activeElement){var s=U;if(a.has(s))return}qa(e)&&o===Ka(e.value)||e.type==="date"&&!o&&!e.value||o!==e.value&&(e.value=o??"")})}function qa(e){var t=e.type;return t==="number"||t==="range"}function Ka(e){return e===""?null:+e}function Xi(e=!1){const t=ze,r=t.l.u;if(!r)return;let a=()=>ur(t.s);if(e){let o=0,s={};const i=fa(()=>{let l=!1;const c=t.s;for(const d in c)c[d]!==s[d]&&(s[d]=c[d],l=!0);return l&&o++,o});a=()=>n(i)}r.b.length&&pi(()=>{qo(t,a),Ja(r.b)}),Ke(()=>{const o=Wr(()=>r.m.map(ys));return()=>{for(const s of o)typeof s=="function"&&s()}}),r.a.length&&Ke(()=>{qo(t,a),Ja(r.a)})}function qo(e,t){if(e.l.s)for(const r of e.l.s)n(r);t()}const Qi={get(e,t){if(!e.exclude.includes(t))return n(e.version),t in e.special?e.special[t]():e.props[t]},set(e,t,r){if(!(t in e.special)){var a=te;try{xt(e.parent_effect),e.special[t]=Yt({get[t](){return e.props[t]}},t,Qo)}finally{xt(a)}}return e.special[t](r),Oo(e.version),!0},getOwnPropertyDescriptor(e,t){if(!e.exclude.includes(t)&&t in e.props)return{enumerable:!0,configurable:!0,value:e.props[t]}},deleteProperty(e,t){return e.exclude.includes(t)||(e.exclude.push(t),Oo(e.version)),!0},has(e,t){return e.exclude.includes(t)?!1:t in e.props},ownKeys(e){return Reflect.ownKeys(e.props).filter(t=>!e.exclude.includes(t))}};function ge(e,t){return new Proxy({props:e,exclude:t,special:{},version:or(0),parent_effect:te},Qi)}const el={get(e,t){let r=e.props.length;for(;r--;){let a=e.props[r];if(qr(a)&&(a=a()),typeof a=="object"&&a!==null&&t in a)return a[t]}},set(e,t,r){let a=e.props.length;for(;a--;){let o=e.props[a];qr(o)&&(o=o());const s=er(o,t);if(s&&s.set)return s.set(r),!0}return!1},getOwnPropertyDescriptor(e,t){let r=e.props.length;for(;r--;){let a=e.props[r];if(qr(a)&&(a=a()),typeof a=="object"&&a!==null&&t in a){const o=er(a,t);return o&&!o.configurable&&(o.configurable=!0),o}}},has(e,t){if(t===Bt||t===on)return!1;for(let r of e.props)if(qr(r)&&(r=r()),r!=null&&t in r)return!0;return!1},ownKeys(e){const t=[];for(let r of e.props)if(qr(r)&&(r=r()),!!r){for(const a in r)t.includes(a)||t.push(a);for(const a of Object.getOwnPropertySymbols(r))t.includes(a)||t.push(a)}return t}};function Ae(...e){return new Proxy({props:e},el)}function Yt(e,t,r,a){var C;var o=!da||(r&cs)!==0,s=(r&ds)!==0,i=(r&us)!==0,l=a,c=!0,d=()=>(c&&(c=!1,l=i?Wr(a):a),l);let f;if(s){var g=Bt in e||on in e;f=((C=er(e,t))==null?void 0:C.set)??(g&&t in e?E=>e[t]=E:void 0)}var b,y=!1;s?[b,y]=Vs(()=>e[t]):b=e[t],b===void 0&&a!==void 0&&(b=d(),f&&(o&&Is(),f(b)));var p;if(o?p=()=>{var E=e[t];return E===void 0?d():(c=!0,E)}:p=()=>{var E=e[t];return E!==void 0&&(l=void 0),E===void 0?l:E},o&&(r&Qo)===0)return p;if(f){var $=e.$$legacy;return(function(E,O){return arguments.length>0?((!o||!O||$||y)&&f(O?p():E),E):p()})}var x=!1,v=((r&ls)!==0?fa:yo)(()=>(x=!1,p()));s&&n(v);var A=te;return(function(E,O){if(arguments.length>0){const D=O?n(v):o&&s?me(E):E;return S(v,D),x=!0,l!==void 0&&(l=D),E}return Ht&&x||(A.f&lt)!==0?v.v:n(v)})}const tl="/aonx/v1";function Zn(){return sessionStorage.getItem("kagami_session_token")||""}async function _a(e,t,r=null,a={}){const o=a.token||Zn(),s={"Content-Type":"application/json"};o&&(s.Authorization=`Bearer ${o}`);const i=await fetch(tl+t,{method:e,headers:s,body:r?JSON.stringify(r):null});if(a.raw)return i;let l=null;const c=await i.text();if(c)try{l=JSON.parse(c)}catch{l=c}return{status:i.status,data:l}}const rt=(e,t)=>_a("GET",e,null,t),sr=(e,t,r)=>_a("POST",e,t,r),rl=(e,t,r)=>_a("PATCH",e,t,r);async function al(){const e=await fetch("/metrics");if(!e.ok)return new Map;const t=await e.text(),r=new Map;for(const a of t.split(`
`)){if(!a||a.startsWith("#"))continue;const o=a.match(/^([a-zA-Z_:][a-zA-Z0-9_:]*(?:\{[^}]*\})?)\s+([\d.eE+-]+(?:nan|inf)?)/i);o&&r.set(o[1],parseFloat(o[2]))}return r}const ol=["kagami_auth_token","kagami_session_token","kagami_username","kagami_acl"];for(const e of ol)localStorage.removeItem(e);const se=me({authToken:sessionStorage.getItem("kagami_auth_token")||"",sessionToken:sessionStorage.getItem("kagami_session_token")||"",username:sessionStorage.getItem("kagami_username")||"",acl:sessionStorage.getItem("kagami_acl")||"",loggedIn:!!sessionStorage.getItem("kagami_session_token"),logoutReason:""});function co(){sessionStorage.setItem("kagami_auth_token",se.authToken),sessionStorage.setItem("kagami_session_token",se.sessionToken),sessionStorage.setItem("kagami_username",se.username),sessionStorage.setItem("kagami_acl",se.acl)}async function nl(e,t){var s,i;const r=await sr("/auth/login",{username:e,password:t});if(r.status!==201)return{ok:!1,error:((s=r.data)==null?void 0:s.reason)||"Login failed"};const a=r.data.token,o=await sr("/session",{client_name:"kagami-admin",client_version:"1.0",hdid:"admin-dashboard-"+crypto.randomUUID().slice(0,8),auth:a});return o.status!==201?{ok:!1,error:((i=o.data)==null?void 0:i.reason)||"Session creation failed"}:(se.authToken=a,se.sessionToken=o.data.token,se.username=r.data.username,se.acl=r.data.acl,se.loggedIn=!0,co(),{ok:!0})}async function sl(){se.authToken&&await sr("/auth/logout",null,{token:se.authToken}),se.authToken="",se.sessionToken="",se.username="",se.acl="",se.loggedIn=!1;for(const e of["kagami_auth_token","kagami_session_token","kagami_username","kagami_acl"])sessionStorage.removeItem(e)}async function il(){if(!(!se.sessionToken||(await _a("PATCH","/session",null)).status===200)){if(se.authToken){const t=await sr("/session",{client_name:"kagami-admin",client_version:"1.0",hdid:"admin-dashboard-"+crypto.randomUUID().slice(0,8),auth:se.authToken});if(t.status===201){se.sessionToken=t.data.token,co();return}}se.authToken="",se.sessionToken="",se.username="",se.acl="",se.loggedIn=!1,se.logoutReason="Session expired. The server may have restarted.",co(),window.location.hash="#/login"}}let Qr=null;function ll(){Qr||(Qr=setInterval(il,6e4))}function Ko(){Qr&&(clearInterval(Qr),Qr=null)}Fs();/**
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
 */const cl={xmlns:"http://www.w3.org/2000/svg",width:24,height:24,viewBox:"0 0 24 24",fill:"none",stroke:"currentColor","stroke-width":2,"stroke-linecap":"round","stroke-linejoin":"round"};/**
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
 */const dl=e=>{for(const t in e)if(t.startsWith("aria-")||t==="role"||t==="title")return!0;return!1};/**
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
 */const Jo=(...e)=>e.filter((t,r,a)=>!!t&&t.trim()!==""&&a.indexOf(t)===r).join(" ").trim();var ul=$i("<svg><!><!></svg>");function Ne(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]),a=ge(r,["name","color","size","strokeWidth","absoluteStrokeWidth","iconNode"]);Je(t,!1);let o=Yt(t,"name",8,void 0),s=Yt(t,"color",8,"currentColor"),i=Yt(t,"size",8,24),l=Yt(t,"strokeWidth",8,2),c=Yt(t,"absoluteStrokeWidth",8,!1),d=Yt(t,"iconNode",24,()=>[]);Xi();var f=ul();Ho(f,(y,p,$)=>({...cl,...y,...a,width:i(),height:i(),stroke:s(),"stroke-width":p,class:$}),[()=>dl(a)?void 0:{"aria-hidden":"true"},()=>(ur(c()),ur(l()),ur(i()),Wr(()=>c()?Number(l())*24/Number(i()):l())),()=>(ur(Jo),ur(o()),ur(r),Wr(()=>Jo("lucide-icon","lucide",o()?`lucide-${o()}`:"",r.class)))]);var g=u(f);Se(g,1,d,$e,(y,p)=>{var $=pe(()=>ho(n(p),2));let x=()=>n($)[0],v=()=>n($)[1];var A=ue(),C=ie(A);ji(C,x,!0,(E,O)=>{Ho(E,()=>({...v()}))}),m(y,A)});var b=_(g);ke(b,t,"default",{}),m(e,f),Ge()}function fl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["circle",{cx:"12",cy:"12",r:"10"}],["path",{d:"M4.929 4.929 19.07 19.071"}]];Ne(e,Ae({name:"ban"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function vl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M15 3h6v6"}],["path",{d:"M10 14 21 3"}],["path",{d:"M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"}]];Ne(e,Ae({name:"external-link"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function pl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M12 3q1 4 4 6.5t3 5.5a1 1 0 0 1-14 0 5 5 0 0 1 1-3 1 1 0 0 0 5 0c0-2-1.5-3-1.5-5q0-2 2.5-4"}]];Ne(e,Ae({name:"flame"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function hl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"m6 14 1.5-2.9A2 2 0 0 1 9.24 10H20a2 2 0 0 1 1.94 2.5l-1.54 6a2 2 0 0 1-1.95 1.5H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H18a2 2 0 0 1 2 2v2"}]];Ne(e,Ae({name:"folder-open"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function Xn(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M2.586 17.414A2 2 0 0 0 2 18.828V21a1 1 0 0 0 1 1h3a1 1 0 0 0 1-1v-1a1 1 0 0 1 1-1h1a1 1 0 0 0 1-1v-1a1 1 0 0 1 1-1h.172a2 2 0 0 0 1.414-.586l.814-.814a6.5 6.5 0 1 0-4-4z"}],["circle",{cx:"16.5",cy:"7.5",r:".5",fill:"currentColor"}]];Ne(e,Ae({name:"key-round"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function _l(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["rect",{width:"7",height:"9",x:"3",y:"3",rx:"1"}],["rect",{width:"7",height:"5",x:"14",y:"3",rx:"1"}],["rect",{width:"7",height:"9",x:"14",y:"12",rx:"1"}],["rect",{width:"7",height:"5",x:"3",y:"16",rx:"1"}]];Ne(e,Ae({name:"layout-dashboard"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function xl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"m16 17 5-5-5-5"}],["path",{d:"M21 12H9"}],["path",{d:"M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"}]];Ne(e,Ae({name:"log-out"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function bl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M14.106 5.553a2 2 0 0 0 1.788 0l3.659-1.83A1 1 0 0 1 21 4.619v12.764a1 1 0 0 1-.553.894l-4.553 2.277a2 2 0 0 1-1.788 0l-4.212-2.106a2 2 0 0 0-1.788 0l-3.659 1.83A1 1 0 0 1 3 19.381V6.618a1 1 0 0 1 .553-.894l4.553-2.277a2 2 0 0 1 1.788 0z"}],["path",{d:"M15 5.764v15"}],["path",{d:"M9 3.236v15"}]];Ne(e,Ae({name:"map"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function ml(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M4 5h16"}],["path",{d:"M4 12h16"}],["path",{d:"M4 19h16"}]];Ne(e,Ae({name:"menu"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function gl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M22 17a2 2 0 0 1-2 2H6.828a2 2 0 0 0-1.414.586l-2.202 2.202A.71.71 0 0 1 2 21.286V5a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2z"}]];Ne(e,Ae({name:"message-square"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function yl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M20.985 12.486a9 9 0 1 1-9.473-9.472c.405-.022.617.46.402.803a6 6 0 0 0 8.268 8.268c.344-.215.825-.004.803.401"}]];Ne(e,Ae({name:"moon"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function Qn(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M5 12h14"}],["path",{d:"M12 5v14"}]];Ne(e,Ae({name:"plus"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function wl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M12 2v10"}],["path",{d:"M18.4 6.6a9 9 0 1 1-12.77.04"}]];Ne(e,Ae({name:"power"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function es(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z"}],["path",{d:"M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7"}],["path",{d:"M7 3v4a1 1 0 0 0 1 1h7"}]];Ne(e,Ae({name:"save"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function kl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"m21 21-4.34-4.34"}],["circle",{cx:"11",cy:"11",r:"8"}]];Ne(e,Ae({name:"search"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function $l(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M9.671 4.136a2.34 2.34 0 0 1 4.659 0 2.34 2.34 0 0 0 3.319 1.915 2.34 2.34 0 0 1 2.33 4.033 2.34 2.34 0 0 0 0 3.831 2.34 2.34 0 0 1-2.33 4.033 2.34 2.34 0 0 0-3.319 1.915 2.34 2.34 0 0 1-4.659 0 2.34 2.34 0 0 0-3.32-1.915 2.34 2.34 0 0 1-2.33-4.033 2.34 2.34 0 0 0 0-3.831A2.34 2.34 0 0 1 6.35 6.051a2.34 2.34 0 0 0 3.319-1.915"}],["circle",{cx:"12",cy:"12",r:"3"}]];Ne(e,Ae({name:"settings"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function Sl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"}]];Ne(e,Ae({name:"shield"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function El(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["circle",{cx:"12",cy:"12",r:"4"}],["path",{d:"M12 2v2"}],["path",{d:"M12 20v2"}],["path",{d:"m4.93 4.93 1.41 1.41"}],["path",{d:"m17.66 17.66 1.41 1.41"}],["path",{d:"M2 12h2"}],["path",{d:"M20 12h2"}],["path",{d:"m6.34 17.66-1.41 1.41"}],["path",{d:"m19.07 4.93-1.41 1.41"}]];Ne(e,Ae({name:"sun"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function uo(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M10 11v6"}],["path",{d:"M14 11v6"}],["path",{d:"M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6"}],["path",{d:"M3 6h18"}],["path",{d:"M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"}]];Ne(e,Ae({name:"trash-2"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function Al(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"}],["path",{d:"M16 3.128a4 4 0 0 1 0 7.744"}],["path",{d:"M22 21v-2a4 4 0 0 0-3-3.87"}],["circle",{cx:"9",cy:"7",r:"4"}]];Ne(e,Ae({name:"users"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function Nl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
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
 */const a=[["path",{d:"M18 6 6 18"}],["path",{d:"m6 6 12 12"}]];Ne(e,Ae({name:"x"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);ke(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}const Tl=(e,t=gr)=>{var r=ue(),a=ie(r);zi(a,t,(o,s)=>{s(o,{size:15,strokeWidth:1.5})}),m(e,r)};var Il=w("<button><!> </button>"),Pl=w('<div class="fixed inset-0 bg-black/40 z-30 md:hidden"></div>'),Ml=w(`<button class="md:hidden fixed top-3 left-3 z-50 p-2 bg-(--color-surface-2) border border-(--color-border)" aria-label="Toggle navigation"><!></button> <aside><div class="px-4 py-3 border-b border-(--color-border)"><h1 class="text-sm font-semibold tracking-wide uppercase text-(--color-accent)">Kagami</h1> <p class="text-xs text-(--color-text-muted) mt-0.5"> </p></div> <nav class="flex-1 overflow-y-auto py-1"><!> <a href="/grafana/" target="_blank" rel="noopener" class="w-full flex items-center gap-2.5 px-4 py-1.5 text-sm text-(--color-text-secondary)
             hover:bg-(--color-surface-2) hover:text-(--color-text-primary)"><!> Monitoring</a></nav> <div class="px-3 py-2 border-t border-(--color-border) flex items-center justify-between"><div class="flex items-center gap-1"><button class="flex items-center gap-1.5 px-2 py-1 text-xs text-(--color-text-muted) hover:text-(--color-text-primary) transition-colors"><!> Logout</button> <button class="flex items-center gap-1 px-2 py-1 text-xs text-(--color-text-muted) hover:text-red-500 transition-colors" title="Shut down server"><!></button></div> <button class="p-1 text-(--color-text-muted) hover:text-(--color-text-primary) transition-colors" aria-label="Toggle theme"><!></button></div></aside> <!>`,1);function Ol(e,t){Je(t,!0);let r=Yt(t,"currentPage",3,"dashboard"),a=W(!1),o=W(!document.documentElement.classList.contains("light"));function s(){S(o,!n(o)),document.documentElement.classList.toggle("light",!n(o)),localStorage.setItem("kagami_theme",n(o)?"dark":"light")}Ke(()=>{localStorage.getItem("kagami_theme")==="light"&&(S(o,!1),document.documentElement.classList.add("light"))});const i=[{page:"dashboard",label:"Dashboard",icon:_l},{page:"sessions",label:"Players",icon:Al},{page:"traffic",label:"Traffic",icon:gl},{page:"config",label:"Config",icon:$l},{page:"bans",label:"Bans",icon:fl},{page:"moderation",label:"Moderation",icon:Sl},{page:"areas",label:"Areas",icon:bl},{page:"users",label:"Accounts",icon:Xn},{page:"firewall",label:"Firewall",icon:pl},{page:"content",label:"Content",icon:hl}];function l(N){window.location.hash="#/"+N,S(a,!1)}async function c(){await sl(),window.location.hash="#/login"}async function d(){confirm("Shut down the server? All connections will be closed.")&&await sr("/admin/stop")}var f=Ml(),g=ie(f),b=u(g);{var y=N=>{Nl(N,{size:18})},p=N=>{ml(N,{size:18})};J(b,N=>{n(a)?N(y):N(p,-1)})}var $=_(g,2),x=u($),v=_(u(x),2),A=u(v),C=_(x,2),E=u(C);Se(E,17,()=>i,$e,(N,Y)=>{let ee=()=>n(Y).page,ce=()=>n(Y).label,ae=()=>n(Y).icon;var k=Il(),L=u(k);Tl(L,ae);var q=_(L);F(()=>{Ie(k,1,`w-full flex items-center gap-2.5 px-4 py-1.5 text-sm transition-colors
               ${r()===ee()?"bg-(--color-surface-2) text-(--color-accent) font-medium":"text-(--color-text-secondary) hover:bg-(--color-surface-2) hover:text-(--color-text-primary)"}`),I(q,` ${ce()??""}`)}),Z("click",k,()=>l(ee())),m(N,k)});var O=_(E,2),D=u(O);vl(D,{size:15,strokeWidth:1.5});var T=_(C,2),z=u(T),P=u(z),H=u(P);xl(H,{size:13,strokeWidth:1.5});var B=_(P,2),K=u(B);wl(K,{size:13,strokeWidth:1.5});var R=_(z,2),j=u(R);{var V=N=>{El(N,{size:14,strokeWidth:1.5})},G=N=>{yl(N,{size:14,strokeWidth:1.5})};J(j,N=>{n(o)?N(V):N(G,-1)})}var le=_($,2);{var M=N=>{var Y=Pl();Z("click",Y,()=>S(a,!1)),m(N,Y)};J(le,N=>{n(a)&&N(M)})}F(()=>{Ie($,1,`fixed md:static inset-y-0 left-0 z-40 w-52 bg-(--color-surface-1) border-r border-(--color-border)
         flex flex-col transition-transform duration-150
         ${n(a)?"translate-x-0":"-translate-x-full"} md:translate-x-0`),I(A,`${se.username??""} · ${se.acl??""}`)}),Z("click",g,()=>S(a,!n(a))),Z("click",P,c),Z("click",B,d),Z("click",R,s),m(e,f),Ge()}Ut(["click"]);var Cl=w('<div class="text-xs text-amber-500 bg-amber-500/10 border border-amber-500/20 px-3 py-2 mb-4"> </div>'),Ll=w('<div class="text-xs text-red-500 bg-red-500/10 border border-red-500/20 px-3 py-2"> </div>'),Rl=w(`<div class="min-h-screen bg-(--color-surface-0) flex items-center justify-center px-4"><div class="w-full max-w-xs"><div class="bg-(--color-surface-1) border border-(--color-border) p-6"><div class="flex items-center gap-2 mb-1"><!> <h1 class="text-lg font-semibold tracking-wide uppercase">Kagami</h1></div> <p class="text-xs text-(--color-text-muted) mb-6">Server Administration</p> <!> <form class="space-y-4"><div><label for="username" class="block text-xs font-medium text-(--color-text-secondary) mb-1">Username</label> <input id="username" type="text" required="" autocomplete="username" class="w-full px-3 py-2 bg-(--color-surface-2) border border-(--color-border) text-sm text-(--color-text-primary)
                   placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)" placeholder="root"/></div> <div><label for="password" class="block text-xs font-medium text-(--color-text-secondary) mb-1">Password</label> <input id="password" type="password" required="" autocomplete="current-password" class="w-full px-3 py-2 bg-(--color-surface-2) border border-(--color-border) text-sm text-(--color-text-primary)
                   placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)"/></div> <!> <button type="submit" class="w-full py-2 px-4 bg-(--color-accent) text-(--color-surface-0) text-sm font-medium
                 hover:opacity-80 disabled:opacity-30 transition-opacity"> </button></form></div></div></div>`);function Dl(e,t){Je(t,!0);let r=W(""),a=W(""),o=W(""),s=W(!1);async function i(T){T.preventDefault(),S(o,""),S(s,!0),se.logoutReason="";const z=await nl(n(r),n(a));S(s,!1),z.ok||S(o,z.error,!0)}var l=Rl(),c=u(l),d=u(c),f=u(d),g=u(f);Xn(g,{size:18,strokeWidth:1.5});var b=_(f,4);{var y=T=>{var z=Cl(),P=u(z);F(()=>I(P,se.logoutReason)),m(T,z)};J(b,T=>{se.logoutReason&&T(y)})}var p=_(b,2),$=u(p),x=_(u($),2),v=_($,2),A=_(u(v),2),C=_(v,2);{var E=T=>{var z=Ll(),P=u(z);F(()=>I(P,n(o))),m(T,z)};J(C,T=>{n(o)&&T(E)})}var O=_(C,2),D=u(O);F(()=>{O.disabled=n(s),I(D,n(s)?"Signing in...":"Sign in")}),Un("submit",p,i),nr(x,()=>n(r),T=>S(r,T)),nr(A,()=>n(a),T=>S(a,T)),m(e,l),Ge()}const Tr=(e,t=gr,r=gr,a=gr)=>{var o=zl(),s=u(o),i=u(s),l=_(i),c=u(l),d=_(s,2),f=u(d);F(()=>{I(i,r()),I(c,a()),I(f,t())}),m(e,o)};var zl=w('<div class="bg-(--color-surface-1) p-3"><div class="text-xl font-semibold tabular-nums"> <span class="text-xs text-(--color-text-muted) font-normal"> </span></div> <div class="text-[10px] uppercase tracking-wider text-(--color-text-muted) mt-0.5"> </div></div>'),jl=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Fl=w('<span class="text-xs text-(--color-text-secondary)"> </span>'),Bl=w('<div class="bg-(--color-surface-1) border border-(--color-border) px-4 py-3 flex flex-wrap items-baseline gap-x-3 gap-y-1"><span class="font-semibold text-sm"> </span> <span class="text-xs text-(--color-text-muted)"> </span> <!></div>'),Wl=w('<span class="text-[10px] px-1 py-px bg-cyan-500/15 text-cyan-400 font-medium"> </span>'),Vl=w('<span class="text-[10px] px-1 py-px bg-amber-500/15 text-amber-400 font-medium"> </span>'),Hl=w('<div class="px-4 py-1.5 flex items-center justify-between text-sm"><div class="flex items-center gap-2"><span> </span> <!> <!></div> <span class="text-xs text-(--color-text-muted) tabular-nums"> </span></div>'),Ul=w('<a href="#/sessions" class="text-xs text-(--color-text-muted) hover:text-(--color-text-primary)">View all</a>'),ql=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1"> </td><td class="px-4 py-1 text-(--color-text-secondary)"> </td><td class="px-4 py-1 text-right text-(--color-text-muted) tabular-nums"> </td></tr>'),Kl=w('<tr><td colspan="3" class="px-4 py-4 text-center text-(--color-text-muted) text-xs">No active sessions</td></tr>'),Jl=w('<!> <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-px bg-(--color-border)"><!> <!> <!> <!> <!> <!></div> <div class="grid lg:grid-cols-2 gap-5"><div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="px-4 py-2 border-b border-(--color-border)"><h3 class="text-xs font-semibold uppercase tracking-wide text-(--color-text-muted)">Areas</h3></div> <div class="divide-y divide-(--color-border)"></div></div> <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="px-4 py-2 border-b border-(--color-border) flex items-center justify-between"><h3 class="text-xs font-semibold uppercase tracking-wide text-(--color-text-muted)">Active Sessions</h3> <!></div> <div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-1.5">Name</th><th class="px-4 py-1.5">Area</th><th class="px-4 py-1.5 text-right">Idle</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div></div>',1),Gl=w('<div class="space-y-5"><h2 class="text-lg font-semibold">Dashboard</h2> <!></div>');function Yl(e,t){Je(t,!0);let r=W(null),a=W(me([])),o=W(me([])),s=W(me(new Map)),i=W(!0);async function l(){var v;const[y,p,$,x]=await Promise.all([rt("/server"),rt("/admin/sessions"),rt("/areas"),al()]);y.status===200&&S(r,y.data,!0),p.status===200&&S(a,p.data,!0),$.status===200&&S(o,((v=$.data)==null?void 0:v.areas)||[],!0),S(s,x,!0),S(i,!1)}Ke(()=>{l();const y=setInterval(l,1e4);return()=>clearInterval(y)});function c(y){return n(s).get(y)??0}var d=Gl(),f=_(u(d),2);{var g=y=>{var p=jl();m(y,p)},b=y=>{var p=Jl(),$=ie(p);{var x=M=>{var N=Bl(),Y=u(N),ee=u(Y),ce=_(Y,2),ae=u(ce),k=_(ce,2);{var L=q=>{var Q=Fl(),_e=u(Q);F(()=>I(_e,n(r).description)),m(q,Q)};J(k,q=>{n(r).description&&q(L)})}F(()=>{I(ee,n(r).name),I(ae,`v${n(r).version??""}`)}),m(M,N)};J($,M=>{n(r)&&M(x)})}var v=_($,2),A=u(v);Tr(A,()=>"Players",()=>{var M;return((M=n(r))==null?void 0:M.online)??0},()=>{var M;return"/"+(((M=n(r))==null?void 0:M.max)??"?")});var C=_(A,2);{let M=pe(()=>c("kagami_ws_connections"));Tr(C,()=>"WS Clients",()=>n(M),()=>"")}var E=_(C,2);Tr(E,()=>"REST",()=>n(a).length,()=>"");var O=_(E,2);Tr(O,()=>"Areas",()=>n(o).length,()=>"");var D=_(O,2);{let M=pe(()=>c("kagami_sessions_moderators"));Tr(D,()=>"Mods",()=>n(M),()=>"")}var T=_(D,2);{let M=pe(()=>c("kagami_characters_taken"));Tr(T,()=>"Chars",()=>n(M),()=>"")}var z=_(v,2),P=u(z),H=_(u(P),2);Se(H,21,()=>n(o),$e,(M,N)=>{var Y=Hl(),ee=u(Y),ce=u(ee),ae=u(ce),k=_(ce,2);{var L=xe=>{var Me=Wl(),fe=u(Me);F(()=>I(fe,n(N).status)),m(xe,Me)};J(k,xe=>{n(N).status&&n(N).status!=="IDLE"&&xe(L)})}var q=_(k,2);{var Q=xe=>{var Me=Vl(),fe=u(Me);F(()=>I(fe,n(N).locked)),m(xe,Me)};J(q,xe=>{n(N).locked&&n(N).locked!=="FREE"&&xe(Q)})}var _e=_(ee,2),Te=u(_e);F(()=>{I(ae,n(N).name),I(Te,n(N).players??0)}),m(M,Y)});var B=_(P,2),K=u(B),R=_(u(K),2);{var j=M=>{var N=Ul();m(M,N)};J(R,M=>{n(a).length>15&&M(j)})}var V=_(K,2),G=u(V),le=_(u(G));Se(le,21,()=>n(a).slice(0,15),$e,(M,N)=>{var Y=ql(),ee=u(Y),ce=u(ee),ae=_(ee),k=u(ae),L=_(ae),q=u(L);F(()=>{I(ce,n(N).display_name||"(anon)"),I(k,n(N).area),I(q,`${n(N).idle_seconds??""}s`)}),m(M,Y)},M=>{var N=Kl();m(M,N)}),m(y,p)};J(f,y=>{n(i)?y(g):y(b,-1)})}m(e,d),Ge()}var Zl=w('<span class="text-(--color-text-muted) font-mono truncate max-w-20"> </span>'),Xl=w('<button class="w-full px-3 py-1.5 text-left text-xs hover:bg-(--color-surface-2) flex justify-between gap-2"><span class="text-(--color-text-secondary) truncate"> </span> <!></button>'),Ql=w('<div class="absolute top-full left-0 right-0 z-10 mt-px bg-(--color-surface-1) border border-(--color-border) max-h-60 overflow-y-auto"></div>'),ec=w('<div class="text-xs text-red-500 bg-red-500/10 border border-red-500/20 px-3 py-2"> </div>'),tc=w("<pre> </pre>"),rc=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),ac=w('<span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>'),oc=w('<button><span class="truncate"> </span> <!></button>'),nc=w('<span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>'),sc=w('<span class="text-[10px] text-(--color-text-muted) font-mono truncate ml-2 max-w-16"> </span>'),ic=w('<button><span class="truncate"> </span> <!></button>'),lc=w('<div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)"></div>'),cc=w('<p class="text-(--color-text-muted) text-sm">Select a key to edit</p>'),dc=w('<input type="checkbox" class="accent-(--color-accent)"/>'),uc=w('<input type="number" step="any" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"/>'),fc=w('<textarea rows="2" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"></textarea>'),vc=w('<input type="text" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)"/>'),pc=w('<div class="flex items-center gap-2"><span class="text-[10px] text-(--color-text-muted) w-5 text-right tabular-nums"></span> <!> <button class="p-1 text-(--color-text-muted) hover:text-red-400"><!></button></div>'),hc=w('<div class="space-y-1"><!> <button class="flex items-center gap-1 mt-2 px-2 py-1 text-xs text-(--color-text-muted) hover:text-(--color-text-primary) border border-dashed border-(--color-border) hover:border-(--color-border-active)"><!> Add item</button></div>'),_c=w('<input type="checkbox" class="accent-(--color-accent)"/>'),xc=w('<input type="number" step="any" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"/>'),bc=w('<input type="text" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)"/>'),mc=w('<div class="flex items-center gap-3"><span class="text-xs text-(--color-text-secondary) w-44 shrink-0 truncate"> </span> <!></div>'),gc=w('<div class="space-y-2"></div>'),yc=w('<input type="checkbox" class="accent-(--color-accent)"/> <span class="text-sm"> </span>',1),wc=w('<input type="number" step="any" class="w-48 px-2 py-1 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"/>'),kc=w('<input type="text" class="flex-1 px-2 py-1 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)"/>'),$c=w('<div class="flex items-center gap-3"><!></div>'),Sc=w('<h3 class="text-xs font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3"> </h3> <!>',1),Ec=w('<div class="flex border border-(--color-border) overflow-hidden" style="height: 65vh"><div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)"></div> <!> <div class="flex-1 overflow-y-auto bg-(--color-surface-0) p-4"><!></div></div>'),Ac=w(`<div class="space-y-3"><div class="flex items-center justify-between flex-wrap gap-2"><h2 class="text-lg font-semibold">Configuration</h2> <div class="flex gap-px items-center"><div class="relative"><!> <input type="text" placeholder="Search keys..." class="pl-8 pr-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
                 placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-48"/> <!></div> <button class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-accent) text-(--color-surface-0) hover:opacity-80 disabled:opacity-30"><!> </button></div></div> <!> <!> <!></div>`);function Nc(e,t){Je(t,!0);let r=W(null),a=W(null),o=W(!0),s=W(!1),i=W(""),l=W(""),c=W(""),d=W(""),f=W(me([]));async function g(){var L;S(o,!0);const k=await rt("/admin/config");k.status===200?(S(r,k.data,!0),S(a,JSON.parse(JSON.stringify(k.data)),!0),S(f,[],!0)):S(c,((L=k.data)==null?void 0:L.reason)||"Failed to load config",!0),S(o,!1)}Ke(()=>{g()});function b(k){let L=n(r);for(let q=0;q<k;q++){if(!L||typeof L!="object")return null;L=L[n(f)[q]]}return L}function y(k,L){S(f,[...n(f).slice(0,k),L],!0)}function p(k){return k!==null&&typeof k=="object"&&!Array.isArray(k)}function $(k,L){if(k===L)return{};if(typeof L!="object"||L===null||Array.isArray(L))return JSON.stringify(L)!==JSON.stringify(k)?L:{};const q={};for(const Q of Object.keys(L))if(p(L[Q])&&p(k==null?void 0:k[Q])){const _e=$(k[Q],L[Q]);Object.keys(_e).length>0&&(q[Q]=_e)}else JSON.stringify(L[Q])!==JSON.stringify(k==null?void 0:k[Q])&&(q[Q]=L[Q]);return q}async function x(){var q,Q;S(c,""),S(i,"");const k=$(n(a),n(r));if(typeof k=="object"&&Object.keys(k).length===0){S(i,"No changes."),S(l,"info");return}S(s,!0);const L=await rl("/admin/config",typeof k=="object"?k:n(r));S(s,!1),L.status===200?(S(i,((q=L.data)==null?void 0:q.reload_summary)||"Saved and applied.",!0),S(l,"ok"),S(a,JSON.parse(JSON.stringify(n(r))),!0)):S(c,((Q=L.data)==null?void 0:Q.reason)||"Failed",!0)}function v(k,L=""){const q=[];if(!k||typeof k!="object")return q;for(const[Q,_e]of Object.entries(k)){const Te=L?`${L}/${Q}`:Q;q.push({path:Te,key:Q,value:_e}),p(_e)&&q.push(...v(_e,Te))}return q}let A=pe(()=>{if(!n(d)||!n(r))return[];const k=n(d).toLowerCase();return v(n(r)).filter(L=>L.path.toLowerCase().includes(k)||typeof L.value!="object"&&String(L.value).toLowerCase().includes(k)).slice(0,30)});function C(k){const L=k.split("/");S(f,[],!0);let q=n(r);for(const Q of L)if(q&&p(q)&&Q in q)S(f,[...n(f),Q],!0),q=q[Q];else break;S(d,"")}function E(k){if(k.length>0){const L=k[k.length-1];typeof L=="string"?k.push(""):typeof L=="number"?k.push(0):typeof L=="boolean"?k.push(!1):k.push("")}else k.push("")}function O(k,L){k.splice(L,1)}var D=Ac(),T=u(D),z=_(u(T),2),P=u(z),H=u(P);kl(H,{size:13,class:"absolute left-2.5 top-1/2 -translate-y-1/2 text-(--color-text-muted)",strokeWidth:1.5});var B=_(H,2),K=_(B,2);{var R=k=>{var L=Ql();Se(L,21,()=>n(A),$e,(q,Q)=>{var _e=Xl(),Te=u(_e),xe=u(Te),Me=_(Te,2);{var fe=be=>{var oe=Zl(),He=u(oe);F(Ye=>I(He,Ye),[()=>String(n(Q).value)]),m(be,oe)},ve=pe(()=>!p(n(Q).value));J(Me,be=>{n(ve)&&be(fe)})}F(()=>I(xe,n(Q).path)),Z("click",_e,()=>C(n(Q).path)),m(q,_e)}),m(k,L)};J(K,k=>{n(A).length>0&&k(R)})}var j=_(P,2),V=u(j);es(V,{size:13,strokeWidth:1.5});var G=_(V),le=_(T,2);{var M=k=>{var L=ec(),q=u(L);F(()=>I(q,n(c))),m(k,L)};J(le,k=>{n(c)&&k(M)})}var N=_(le,2);{var Y=k=>{var L=tc(),q=u(L);F(()=>{Ie(L,1,`text-xs ${n(l)==="ok"?"text-emerald-400 bg-emerald-400/10 border-emerald-400/20":"text-(--color-text-secondary) bg-(--color-surface-2) border-(--color-border)"} border px-3 py-2 whitespace-pre-wrap`),I(q,n(i))}),m(k,L)};J(N,k=>{n(i)&&k(Y)})}var ee=_(N,2);{var ce=k=>{var L=rc();m(k,L)},ae=k=>{var L=Ec(),q=u(L);Se(q,21,()=>Object.keys(n(r)),$e,(fe,ve)=>{const be=pe(()=>n(r)[n(ve)]);var oe=oc(),He=u(oe),Ye=u(He),Ot=_(He,2);{var ir=Ze=>{var Ar=ac();m(Ze,Ar)},lr=pe(()=>p(n(be))||Array.isArray(n(be)));J(Ot,Ze=>{n(lr)&&Ze(ir)})}F(()=>{Ie(oe,1,`w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                   ${n(f)[0]===n(ve)?"bg-(--color-surface-2) text-(--color-accent) font-medium":"text-(--color-text-secondary)"}`),I(Ye,n(ve))}),Z("click",oe,()=>y(0,n(ve))),m(fe,oe)});var Q=_(q,2);Se(Q,17,()=>n(f),$e,(fe,ve,be)=>{const oe=pe(()=>b(be)),He=pe(()=>{var Ze;return(Ze=n(oe))==null?void 0:Ze[n(ve)]});var Ye=ue(),Ot=ie(Ye);{var ir=Ze=>{var Ar=lc();Se(Ar,21,()=>Object.keys(n(He)),$e,(Da,Nr)=>{const De=pe(()=>n(He)[n(Nr)]);var at=ic(),Ct=u(at),qt=u(Ct),Kt=_(Ct,2);{var bt=Oe=>{var mt=nc();m(Oe,mt)},de=pe(()=>p(n(De))||Array.isArray(n(De))),ye=Oe=>{var mt=sc(),Jt=u(mt);F(Lt=>I(Jt,Lt),[()=>JSON.stringify(n(De))]),m(Oe,mt)};J(Kt,Oe=>{n(de)?Oe(bt):Oe(ye,-1)})}F(()=>{Ie(at,1,`w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                       ${n(f)[be+1]===n(Nr)?"bg-(--color-surface-2) text-(--color-accent) font-medium":"text-(--color-text-secondary)"}`),I(qt,n(Nr))}),Z("click",at,()=>y(be+1,n(Nr))),m(Da,at)}),m(Ze,Ar)},lr=pe(()=>p(n(He)));J(Ot,Ze=>{n(lr)&&Ze(ir)})}m(fe,Ye)});var _e=_(Q,2),Te=u(_e);{var xe=fe=>{var ve=cc();m(fe,ve)},Me=fe=>{const ve=pe(()=>b(n(f).length-1)),be=pe(()=>n(f)[n(f).length-1]),oe=pe(()=>{var De;return(De=n(ve))==null?void 0:De[n(be)]});var He=Sc(),Ye=ie(He),Ot=u(Ye),ir=_(Ye,2);{var lr=De=>{var at=hc(),Ct=u(at);Se(Ct,17,()=>n(oe),$e,(bt,de,ye)=>{var Oe=pc(),mt=u(Oe);mt.textContent=ye;var Jt=_(mt,2);{var Lt=Xe=>{var je=dc();F(()=>Ua(je,n(de))),Z("change",je,Ce=>n(oe)[ye]=Ce.target.checked),m(Xe,je)},xa=Xe=>{var je=uc();F(()=>dr(je,n(de))),Z("change",je,Ce=>n(oe)[ye]=Number(Ce.target.value)),m(Xe,je)},Hr=Xe=>{var je=fc();F(Ce=>dr(je,Ce),[()=>JSON.stringify(n(de),null,2)]),Z("change",je,Ce=>{try{n(oe)[ye]=JSON.parse(Ce.target.value)}catch{}}),m(Xe,je)},za=Xe=>{var je=vc();F(()=>dr(je,n(de))),Z("input",je,Ce=>n(oe)[ye]=Ce.target.value),m(Xe,je)};J(Jt,Xe=>{typeof n(de)=="boolean"?Xe(Lt):typeof n(de)=="number"?Xe(xa,1):typeof n(de)=="object"?Xe(Hr,2):Xe(za,-1)})}var ba=_(Jt,2),ja=u(ba);uo(ja,{size:12,strokeWidth:1.5}),Z("click",ba,()=>O(n(oe),ye)),m(bt,Oe)});var qt=_(Ct,2),Kt=u(qt);Qn(Kt,{size:12,strokeWidth:1.5}),Z("click",qt,()=>E(n(oe))),m(De,at)},Ze=pe(()=>Array.isArray(n(oe))),Ar=De=>{var at=gc();Se(at,21,()=>Object.entries(n(oe)),$e,(Ct,qt)=>{var Kt=pe(()=>ho(n(qt),2));let bt=()=>n(Kt)[0],de=()=>n(Kt)[1];var ye=ue(),Oe=ie(ye);{var mt=Lt=>{var xa=mc(),Hr=u(xa),za=u(Hr),ba=_(Hr,2);{var ja=Ce=>{var gt=_c();F(()=>Ua(gt,de())),Z("change",gt,Ur=>n(oe)[bt()]=Ur.target.checked),m(Ce,gt)},Xe=Ce=>{var gt=xc();F(()=>dr(gt,de())),Z("change",gt,Ur=>n(oe)[bt()]=Number(Ur.target.value)),m(Ce,gt)},je=Ce=>{var gt=bc();F(()=>dr(gt,de()??"")),Z("input",gt,Ur=>n(oe)[bt()]=Ur.target.value),m(Ce,gt)};J(ba,Ce=>{typeof de()=="boolean"?Ce(ja):typeof de()=="number"?Ce(Xe,1):Ce(je,-1)})}F(()=>{Ta(Hr,"title",bt()),I(za,bt())}),m(Lt,xa)},Jt=pe(()=>!p(de())&&!Array.isArray(de()));J(Oe,Lt=>{n(Jt)&&Lt(mt)})}m(Ct,ye)}),m(De,at)},Da=pe(()=>p(n(oe))),Nr=De=>{var at=$c(),Ct=u(at);{var qt=de=>{var ye=yc(),Oe=ie(ye),mt=_(Oe,2),Jt=u(mt);F(()=>{Ua(Oe,n(oe)),I(Jt,n(oe)?"true":"false")}),Z("change",Oe,Lt=>n(ve)[n(be)]=Lt.target.checked),m(de,ye)},Kt=de=>{var ye=wc();F(()=>dr(ye,n(oe))),Z("change",ye,Oe=>n(ve)[n(be)]=Number(Oe.target.value)),m(de,ye)},bt=de=>{var ye=kc();F(()=>dr(ye,n(oe)??"")),Z("input",ye,Oe=>n(ve)[n(be)]=Oe.target.value),m(de,ye)};J(Ct,de=>{typeof n(oe)=="boolean"?de(qt):typeof n(oe)=="number"?de(Kt,1):de(bt,-1)})}m(De,at)};J(ir,De=>{n(Ze)?De(lr):n(Da)?De(Ar,1):De(Nr,-1)})}F(De=>I(Ot,De),[()=>n(f).join(" / ")]),m(fe,He)};J(Te,fe=>{n(f).length===0?fe(xe):fe(Me,-1)})}m(k,L)};J(ee,k=>{n(o)?k(ce):n(r)&&k(ae,1)})}F(()=>{j.disabled=n(s),I(G,` ${n(s)?"Saving...":"Save & Apply"}`)}),nr(B,()=>n(d),k=>S(d,k)),Z("click",j,x),m(e,D),Ge()}Ut(["click","change","input"]);const ct=(e,t=gr,r=gr)=>{var a=Tc(),o=u(a),s=u(o),i=_(o,2),l=u(i);F(()=>{I(s,t()),I(l,r())}),m(e,a)};var Tc=w('<div><div class="text-(--color-text-muted) text-[10px] uppercase tracking-wider"> </div> <div class="text-(--color-text-primary) font-mono mt-0.5 truncate"> </div></div>'),Ic=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Pc=w('<tr><td class="px-4 py-1.5 font-medium"> </td><td class="px-4 py-1.5 text-(--color-text-secondary)"> </td><td class="px-4 py-1.5 text-(--color-text-secondary)"> </td><td class="px-4 py-1.5 text-(--color-text-muted) hidden sm:table-cell truncate max-w-32"> </td><td class="px-4 py-1.5 text-right text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td><td class="px-4 py-1.5 text-right text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td><td class="px-4 py-1.5 text-right text-(--color-text-muted) tabular-nums"> </td></tr>'),Mc=w('<div class="bg-(--color-surface-1) border border-(--color-border) p-4 space-y-3"><h3 class="text-sm font-semibold"> </h3> <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-4 gap-3 text-xs"><!> <!> <!> <!> <!> <!> <!> <!> <!> <!> <!> <!></div></div>'),Oc=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">Name</th><th class="px-4 py-2">Protocol</th><th class="px-4 py-2">Area</th><th class="px-4 py-2 hidden sm:table-cell">Client</th><th class="px-4 py-2 text-right hidden md:table-cell">Sent</th><th class="px-4 py-2 text-right hidden md:table-cell">Recv</th><th class="px-4 py-2 text-right">Idle</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div> <!>',1),Cc=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Players <span class="text-sm text-(--color-text-muted) font-normal"> </span></h2> <!></div>');function Lc(e,t){Je(t,!0);let r=W(me([])),a=W(!0),o=W(null);async function s(){const p=await rt("/admin/sessions");p.status===200&&S(r,p.data,!0),S(a,!1)}Ke(()=>{s();const p=setInterval(s,5e3);return()=>clearInterval(p)});function i(p){return p<1024?p+" B":p<1024*1024?(p/1024).toFixed(1)+" KB":(p/(1024*1024)).toFixed(1)+" MB"}var l=Cc(),c=u(l),d=_(u(c)),f=u(d),g=_(c,2);{var b=p=>{var $=Ic();m(p,$)},y=p=>{var $=Oc(),x=ie($),v=u(x),A=u(v),C=_(u(A));Se(C,21,()=>n(r),$e,(D,T)=>{var z=Pc(),P=u(z),H=u(P),B=_(P),K=u(B),R=_(B),j=u(R),V=_(R),G=u(V),le=_(V),M=u(le),N=_(le),Y=u(N),ee=_(N),ce=u(ee);F((ae,k)=>{var L;Ie(z,1,`hover:bg-(--color-surface-2)/50 cursor-pointer ${((L=n(o))==null?void 0:L.session_id)===n(T).session_id?"bg-(--color-surface-2)":""}`),I(H,n(T).display_name||"(anon)"),I(K,n(T).protocol),I(j,n(T).area),I(G,n(T).client_software),I(M,ae),I(Y,k),I(ce,`${n(T).idle_seconds??""}s`)},[()=>i(n(T).bytes_sent),()=>i(n(T).bytes_received)]),Z("click",z,()=>{var ae;return S(o,((ae=n(o))==null?void 0:ae.session_id)===n(T).session_id?null:n(T),!0)}),m(D,z)});var E=_(x,2);{var O=D=>{var T=Mc(),z=u(T),P=u(z),H=_(z,2),B=u(H);ct(B,()=>"Session ID",()=>n(o).session_id);var K=_(B,2);ct(K,()=>"Protocol",()=>n(o).protocol);var R=_(K,2);ct(R,()=>"Area",()=>n(o).area);var j=_(R,2);ct(j,()=>"Character",()=>n(o).character_id>=0?"#"+n(o).character_id:"None");var V=_(j,2);ct(V,()=>"HDID",()=>n(o).hardware_id);var G=_(V,2);ct(G,()=>"Client",()=>n(o).client_software);var le=_(G,2);ct(le,()=>"Packets Sent",()=>n(o).packets_sent);var M=_(le,2);ct(M,()=>"Packets Recv",()=>n(o).packets_received);var N=_(M,2);ct(N,()=>"Mod Actions",()=>n(o).mod_actions);var Y=_(N,2);{let ae=pe(()=>i(n(o).bytes_sent));ct(Y,()=>"Bytes Sent",()=>n(ae))}var ee=_(Y,2);{let ae=pe(()=>i(n(o).bytes_received));ct(ee,()=>"Bytes Recv",()=>n(ae))}var ce=_(ee,2);ct(ce,()=>"Idle",()=>n(o).idle_seconds+"s"),F(()=>I(P,n(o).display_name||"(anonymous)")),m(D,T)};J(E,D=>{n(o)&&D(O)})}m(p,$)};J(g,p=>{n(a)?p(b):p(y,-1)})}F(()=>I(f,`(${n(r).length??""})`)),m(e,l),Ge()}Ut(["click"]);var Rc=w('<span class="text-(--color-text-muted) shrink-0"> </span>'),Dc=w('<div class="px-3 py-1 flex gap-2 hover:bg-(--color-surface-2)/50 border-b border-(--color-border)/30"><span class="text-(--color-text-muted) shrink-0 w-16"> </span> <span> </span> <!> <span class="text-amber-400 shrink-0"> </span> <span class="text-(--color-text-primary) break-all"> </span></div>'),zc=w('<div class="px-4 py-12 text-center text-(--color-text-muted)"> </div>'),jc=w(`<div class="space-y-4"><div class="flex items-center justify-between flex-wrap gap-2"><div class="flex items-center gap-2"><h2 class="text-lg font-semibold">Traffic</h2> <span></span></div> <input type="text" placeholder="Filter..." class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
             placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-56"/></div> <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="max-h-[75vh] overflow-y-auto font-mono text-xs leading-relaxed"></div></div></div>`);function Fc(e,t){Je(t,!0);let r=W(me([])),a=W("disconnected"),o=W(""),s=500;function i(){const v=Zn();v&&l(v)}async function l(v){S(a,"connecting");try{const A=await fetch("/aonx/v1/events",{headers:{Authorization:`Bearer ${v}`}});if(!A.ok){S(a,"disconnected");return}S(a,"connected");const C=A.body.getReader(),E=new TextDecoder;let O="";for(;;){const{done:D,value:T}=await C.read();if(D)break;O+=E.decode(T,{stream:!0});const z=O.split(`
`);O=z.pop()||"";let P="",H="";for(const B of z)B.startsWith("event: ")?P=B.slice(7):B.startsWith("data: ")?H=B.slice(6):B===""&&P&&H&&(c(P,H),P="",H="")}}catch(A){console.error("SSE error:",A)}S(a,"disconnected")}function c(v,A){if(!(v!=="ic_message"&&v!=="ooc_message"))try{const C=JSON.parse(A);S(r,[{type:v==="ic_message"?"IC":"OOC",name:C.showname||C.name||C.character||"???",text:C.message||"",time:new Date().toLocaleTimeString("en-US",{hour12:!1}),area:C.area||""},...n(r).slice(0,s-1)],!0)}catch{}}Ke(()=>{i()});let d=pe(()=>n(o)?n(r).filter(v=>v.text.toLowerCase().includes(n(o).toLowerCase())||v.name.toLowerCase().includes(n(o).toLowerCase())||v.area.toLowerCase().includes(n(o).toLowerCase())):n(r));var f=jc(),g=u(f),b=u(g),y=_(u(b),2),p=_(b,2),$=_(g,2),x=u($);Se(x,21,()=>n(d),$e,(v,A)=>{var C=Dc(),E=u(C),O=u(E),D=_(E,2),T=u(D),z=_(D,2);{var P=j=>{var V=Rc(),G=u(V);F(()=>I(G,`[${n(A).area??""}]`)),m(j,V)};J(z,j=>{n(A).area&&j(P)})}var H=_(z,2),B=u(H),K=_(H,2),R=u(K);F(()=>{I(O,n(A).time),Ie(D,1,`shrink-0 w-6 text-center font-bold ${n(A).type==="IC"?"text-cyan-400":"text-emerald-400"}`),I(T,n(A).type),I(B,n(A).name),I(R,n(A).text)}),m(v,C)},v=>{var A=zc(),C=u(A);F(()=>I(C,n(a)==="connected"?"Waiting for messages...":n(a)==="connecting"?"Connecting...":"Not connected")),m(v,A)}),F(()=>Ie(y,1,`w-1.5 h-1.5 ${n(a)==="connected"?"bg-emerald-500":n(a)==="connecting"?"bg-amber-500 animate-pulse":"bg-red-500"}`)),nr(p,()=>n(o),v=>S(o,v)),m(e,f),Ge()}var Bc=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Wc=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-mono text-xs"> </td><td class="px-4 py-1.5 max-w-xs truncate"> </td><td class="px-4 py-1.5 text-(--color-text-secondary) hidden md:table-cell"> </td><td class="px-4 py-1.5 text-(--color-text-muted) text-xs hidden lg:table-cell"> </td><td class="px-4 py-1.5"><span> </span></td><td class="px-4 py-1.5"><button class="text-xs text-red-400 hover:text-red-300">Unban</button></td></tr>'),Vc=w('<tr><td colspan="6" class="px-4 py-8 text-center text-(--color-text-muted) text-sm">No bans found</td></tr>'),Hc=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">IPID</th><th class="px-4 py-2">Reason</th><th class="px-4 py-2 hidden md:table-cell">By</th><th class="px-4 py-2 hidden lg:table-cell">When</th><th class="px-4 py-2">Duration</th><th class="px-4 py-2"></th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),Uc=w(`<div class="space-y-4"><div class="flex items-center justify-between flex-wrap gap-2"><h2 class="text-lg font-semibold">Bans</h2> <form class="flex gap-px"><input type="text" placeholder="Search IPID, HDID, reason..." class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
               placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-56"/> <button type="submit" class="px-3 py-1.5 text-sm bg-(--color-surface-3) border border-(--color-border) text-(--color-text-secondary) hover:text-(--color-text-primary)">Search</button></form></div> <!></div>`);function qc(e,t){Je(t,!0);let r=W(me([])),a=W(""),o=W(!0);async function s(){var A;const x=n(a)?`?query=${encodeURIComponent(n(a))}&limit=100`:"?limit=100",v=await rt("/admin/bans"+x);v.status===200&&S(r,((A=v.data)==null?void 0:A.bans)||[],!0),S(o,!1)}Ke(()=>{s()});async function i(x){confirm(`Unban ${x}?`)&&(await sr("/moderation/actions",{action:"unban",target:x}),s())}function l(x){if(x===-2)return"Permanent";if(x===0)return"Invalidated";const v=Math.floor(x/3600),A=Math.floor(x%3600/60);return v>0?`${v}h ${A}m`:`${A}m`}function c(x){return x?new Date(x*1e3).toLocaleString():""}var d=Uc(),f=u(d),g=_(u(f),2),b=u(g),y=_(f,2);{var p=x=>{var v=Bc();m(x,v)},$=x=>{var v=Hc(),A=u(v),C=u(A),E=_(u(C));Se(E,21,()=>n(r),$e,(O,D)=>{var T=Wc(),z=u(T),P=u(z),H=_(z),B=u(H),K=_(H),R=u(K),j=_(K),V=u(j),G=_(j),le=u(G),M=u(le),N=_(G),Y=u(N);F((ee,ce)=>{I(P,n(D).ipid),I(B,n(D).reason),I(R,n(D).moderator),I(V,ee),Ie(le,1,`text-[10px] px-1 py-px font-medium ${n(D).permanent?"bg-red-500/15 text-red-400":"bg-(--color-surface-3) text-(--color-text-muted)"}`),I(M,ce)},[()=>c(n(D).timestamp),()=>l(n(D).duration)]),Z("click",Y,()=>i(n(D).ipid)),m(O,T)},O=>{var D=Vc();m(O,D)}),m(x,v)};J(y,x=>{n(o)?x(p):x($,-1)})}Un("submit",g,x=>{x.preventDefault(),s()}),nr(b,()=>n(a),x=>S(a,x)),m(e,d),Ge()}Ut(["click"]);var Kc=w('<p class="text-xs mt-2 text-(--color-text-muted)"> </p>'),Jc=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Gc=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-3 py-1.5 text-(--color-text-muted)"> </td><td class="px-3 py-1.5 font-mono"> </td><td class="px-3 py-1.5 text-(--color-text-secondary)"> </td><td class="px-3 py-1.5"><span> </span></td><td class="px-3 py-1.5 text-(--color-text-secondary) max-w-xs truncate hidden md:table-cell"> </td><td class="px-3 py-1.5 text-(--color-text-muted) max-w-xs truncate hidden lg:table-cell"> </td></tr>'),Yc=w('<tr><td colspan="6" class="px-4 py-8 text-center text-(--color-text-muted)">No events</td></tr>'),Zc=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-xs"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-3 py-2">Time</th><th class="px-3 py-2">IPID</th><th class="px-3 py-2">Ch</th><th class="px-3 py-2">Action</th><th class="px-3 py-2 hidden md:table-cell">Message</th><th class="px-3 py-2 hidden lg:table-cell">Reason</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),Xc=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-mono text-xs"> </td><td class="px-4 py-1.5 text-(--color-text-secondary)"> </td><td class="px-4 py-1.5"> </td><td class="px-4 py-1.5"><button class="text-xs text-red-400 hover:text-red-300">Unmute</button></td></tr>'),Qc=w('<tr><td colspan="4" class="px-4 py-8 text-center text-(--color-text-muted)">No active mutes</td></tr>'),ed=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">IPID</th><th class="px-4 py-2">Reason</th><th class="px-4 py-2">Remaining</th><th class="px-4 py-2"></th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),td=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Moderation</h2> <div class="bg-(--color-surface-1) border border-(--color-border) p-4"><h3 class="text-[10px] font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3">Quick Action</h3> <div class="flex flex-wrap gap-px"><input placeholder="IPID / session ID / IP" class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) placeholder:text-(--color-text-muted) w-44 focus:outline-none focus:border-(--color-border-active)"/> <input placeholder="Reason" class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) placeholder:text-(--color-text-muted) flex-1 min-w-24 focus:outline-none focus:border-(--color-border-active)"/> <button class="px-3 py-1.5 text-sm bg-amber-600 text-white hover:bg-amber-500">Kick</button> <button class="px-3 py-1.5 text-sm bg-orange-600 text-white hover:bg-orange-500">Mute</button> <button class="px-3 py-1.5 text-sm bg-red-600 text-white hover:bg-red-500">Ban</button></div> <!></div> <div class="flex gap-4 border-b border-(--color-border)"><button>Audit Log</button> <button>Active Mutes</button></div> <!></div>');function rd(e,t){Je(t,!0);let r=W("audit"),a=W(me([])),o=W(me([])),s=W(!0),i=W(""),l=W(""),c=W("");const d={0:"none",1:"log",2:"censor",3:"drop",4:"mute",5:"kick",6:"ban",7:"perma_ban",none:"none",log:"log",censor:"censor",drop:"drop",mute:"mute",kick:"kick",ban:"ban",perma_ban:"perma_ban"},f={ban:"bg-red-500/15 text-red-400",perma_ban:"bg-red-500/15 text-red-400",kick:"bg-amber-500/15 text-amber-400",mute:"bg-orange-500/15 text-orange-400",censor:"bg-violet-500/15 text-violet-400",drop:"bg-violet-500/15 text-violet-400",log:"bg-(--color-surface-3) text-(--color-text-muted)",none:"bg-(--color-surface-3) text-(--color-text-muted)"};function g(M){return d[String(M)]||String(M)}function b(M){return f[g(M)]||"bg-(--color-surface-3) text-(--color-text-muted)"}async function y(){var N;S(s,!0);const M=await rt("/admin/moderation-events?limit=200");M.status===200&&S(a,((N=M.data)==null?void 0:N.events)||[],!0),S(s,!1)}async function p(){var N;S(s,!0);const M=await rt("/admin/mutes");M.status===200&&S(o,((N=M.data)==null?void 0:N.mutes)||[],!0),S(s,!1)}Ke(()=>{n(r)==="audit"?y():p()});async function $(M){var ee;if(!n(i))return;S(c,"");const N={action:M,target:n(i),reason:n(l)||void 0};M==="mute"&&(N.duration=900);const Y=await sr("/moderation/actions",N);S(c,Y.status===200?`${M} applied`:((ee=Y.data)==null?void 0:ee.reason)||"Failed",!0),S(i,""),S(l,""),n(r)==="mutes"&&p()}async function x(M){await sr("/moderation/actions",{action:"unmute",target:M}),p()}var v=td(),A=_(u(v),2),C=_(u(A),2),E=u(C),O=_(E,2),D=_(O,2),T=_(D,2),z=_(T,2),P=_(C,2);{var H=M=>{var N=Kc(),Y=u(N);F(()=>I(Y,n(c))),m(M,N)};J(P,M=>{n(c)&&M(H)})}var B=_(A,2),K=u(B),R=_(K,2),j=_(B,2);{var V=M=>{var N=Jc();m(M,N)},G=M=>{var N=Zc(),Y=u(N),ee=u(Y),ce=_(u(ee));Se(ce,21,()=>n(a),$e,(ae,k)=>{var L=Gc(),q=u(L),Q=u(q),_e=_(q),Te=u(_e),xe=_(_e),Me=u(xe),fe=_(xe),ve=u(fe),be=u(ve),oe=_(fe),He=u(oe),Ye=_(oe),Ot=u(Ye);F((ir,lr,Ze)=>{I(Q,ir),I(Te,n(k).ipid),I(Me,n(k).channel),Ie(ve,1,`text-[10px] px-1 py-px font-medium ${lr??""}`),I(be,Ze),I(He,n(k).message_sample),I(Ot,n(k).reason)},[()=>new Date(n(k).timestamp_ms).toLocaleString(),()=>b(n(k).action),()=>g(n(k).action)]),m(ae,L)},ae=>{var k=Yc();m(ae,k)}),m(M,N)},le=M=>{var N=ed(),Y=u(N),ee=u(Y),ce=_(u(ee));Se(ce,21,()=>n(o),$e,(ae,k)=>{var L=Xc(),q=u(L),Q=u(q),_e=_(q),Te=u(_e),xe=_(_e),Me=u(xe),fe=_(xe),ve=u(fe);F(be=>{I(Q,n(k).ipid),I(Te,n(k).reason),I(Me,be)},[()=>n(k).seconds_remaining<0?"Permanent":Math.ceil(n(k).seconds_remaining/60)+"m"]),Z("click",ve,()=>x(n(k).ipid)),m(ae,L)},ae=>{var k=Qc();m(ae,k)}),m(M,N)};J(j,M=>{n(s)?M(V):n(r)==="audit"?M(G,1):M(le,-1)})}F(()=>{Ie(K,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)==="audit"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),Ie(R,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)==="mutes"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`)}),nr(E,()=>n(i),M=>S(i,M)),nr(O,()=>n(l),M=>S(l,M)),Z("click",D,()=>$("kick")),Z("click",T,()=>$("mute")),Z("click",z,()=>$("ban")),Z("click",K,()=>S(r,"audit")),Z("click",R,()=>S(r,"mutes")),m(e,v),Ge()}Ut(["click"]);var ad=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),od=w('<span class="px-1 py-px bg-cyan-500/15 text-cyan-400 font-medium"> </span>'),nd=w('<span class="px-1 py-px bg-(--color-surface-3) text-(--color-text-muted)">IDLE</span>'),sd=w('<span class="px-1 py-px bg-amber-500/15 text-amber-400 font-medium"> </span>'),id=w('<button><div class="flex items-center justify-between mb-1"><span class="font-medium text-sm"> </span> <span class="text-xs text-(--color-text-muted) tabular-nums"> </span></div> <div class="flex gap-1 text-[10px]"><!> <!></div></button>'),ld=w('<div><span class="text-(--color-text-muted) text-[10px] uppercase">Background</span><div class="mt-0.5"> </div></div>'),cd=w('<div><span class="text-(--color-text-muted) text-[10px] uppercase">Music</span><div class="mt-0.5"> </div></div>'),dd=w('<div><span class="text-(--color-text-muted) text-[10px] uppercase">HP</span><div class="mt-0.5"> </div></div>'),ud=w('<div class="bg-(--color-surface-1) border border-(--color-border) p-4 space-y-3"><h3 class="text-sm font-semibold"> </h3> <div class="grid grid-cols-2 sm:grid-cols-3 gap-3 text-xs"><!> <!> <!></div></div>'),fd=w('<div class="grid gap-px bg-(--color-border) sm:grid-cols-2 lg:grid-cols-3"></div> <!>',1),vd=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Areas</h2> <!></div>');function pd(e,t){Je(t,!0);let r=W(me([])),a=W(!0),o=W(null);async function s(){var b;const g=await rt("/areas");g.status===200&&S(r,((b=g.data)==null?void 0:b.areas)||[],!0),S(a,!1)}Ke(()=>{s();const g=setInterval(s,1e4);return()=>clearInterval(g)});async function i(g){const b=await rt(`/areas/${g}`);b.status===200&&S(o,b.data,!0)}var l=vd(),c=_(u(l),2);{var d=g=>{var b=ad();m(g,b)},f=g=>{var b=fd(),y=ie(b);Se(y,21,()=>n(r),$e,(x,v)=>{var A=id(),C=u(A),E=u(C),O=u(E),D=_(E,2),T=u(D),z=_(C,2),P=u(z);{var H=j=>{var V=od(),G=u(V);F(()=>I(G,n(v).status)),m(j,V)},B=j=>{var V=nd();m(j,V)};J(P,j=>{n(v).status&&n(v).status!=="IDLE"?j(H):j(B,-1)})}var K=_(P,2);{var R=j=>{var V=sd(),G=u(V);F(()=>I(G,n(v).locked)),m(j,V)};J(K,j=>{n(v).locked&&n(v).locked!=="FREE"&&j(R)})}F(()=>{var j,V;Ie(A,1,`bg-(--color-surface-1) p-3 text-left hover:bg-(--color-surface-2) transition-colors
                 ${((V=(j=n(o))==null?void 0:j.area)==null?void 0:V.id)===n(v).id?"ring-1 ring-(--color-accent) ring-inset":""}`),I(O,n(v).name),I(T,n(v).players??0)}),Z("click",A,()=>i(n(v).id)),m(x,A)});var p=_(y,2);{var $=x=>{var v=ud(),A=u(v),C=u(A),E=_(A,2),O=u(E);{var D=B=>{var K=ld(),R=_(u(K)),j=u(R);F(()=>I(j,n(o).background.name)),m(B,K)};J(O,B=>{n(o).background&&B(D)})}var T=_(O,2);{var z=B=>{var K=cd(),R=_(u(K)),j=u(R);F(()=>I(j,n(o).music.name||"None")),m(B,K)};J(T,B=>{n(o).music&&B(z)})}var P=_(T,2);{var H=B=>{var K=dd(),R=_(u(K)),j=u(R);F(()=>I(j,`Def ${n(o).hp.defense??""} / Pro ${n(o).hp.prosecution??""}`)),m(B,K)};J(P,B=>{n(o).hp&&B(H)})}F(()=>{var B;return I(C,(B=n(o).area)==null?void 0:B.name)}),m(x,v)};J(p,x=>{n(o)&&x($)})}m(g,b)};J(c,g=>{n(a)?g(d):g(f,-1)})}m(e,l),Ge()}Ut(["click"]);var hd=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),_d=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-medium"> </td><td class="px-4 py-1.5"><span> </span></td></tr>'),xd=w('<tr><td colspan="2" class="px-4 py-8 text-center text-(--color-text-muted)">No accounts</td></tr>'),bd=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">Username</th><th class="px-4 py-2">Role</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),md=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Accounts</h2> <!></div>');function gd(e,t){Je(t,!0);let r=W(me([])),a=W(!0);async function o(){var f;const d=await rt("/admin/users");d.status===200&&S(r,((f=d.data)==null?void 0:f.users)||[],!0),S(a,!1)}Ke(()=>{o()});var s=md(),i=_(u(s),2);{var l=d=>{var f=hd();m(d,f)},c=d=>{var f=bd(),g=u(f),b=u(g),y=_(u(b));Se(y,21,()=>n(r),$e,(p,$)=>{var x=_d(),v=u(x),A=u(v),C=_(v),E=u(C),O=u(E);F(()=>{I(A,n($).username),Ie(E,1,`text-[10px] px-1 py-px font-medium
                    ${n($).acl==="SUPER"?"bg-violet-500/15 text-violet-400":n($).acl==="NONE"?"bg-(--color-surface-3) text-(--color-text-muted)":"bg-cyan-500/15 text-cyan-400"}`),I(O,n($).acl)}),m(p,x)},p=>{var $=xd();m(p,$)}),m(d,f)};J(i,d=>{n(a)?d(l):d(c,-1)})}m(e,s),Ge()}var yd=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),wd=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-3 py-1.5 font-mono text-xs"> </td><td class="px-3 py-1.5 text-(--color-text-secondary) max-w-xs truncate"> </td><td class="px-3 py-1.5 text-xs"> </td></tr>'),kd=w('<table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-3 py-2">Target</th><th class="px-3 py-2">Reason</th><th class="px-3 py-2">Expires</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table>'),$d=w('<p class="text-sm text-(--color-text-muted)">No active rules.</p>'),Sd=w('<div class="bg-(--color-surface-1) border border-(--color-border) p-4"><div class="flex items-center gap-2 mb-3"><span class="text-sm font-medium">nftables</span> <span> </span> <span class="text-xs text-(--color-text-muted)"> </span></div> <!></div>'),Ed=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-mono text-xs"> </td><td class="px-4 py-1.5"> </td><td class="px-4 py-1.5"><span> </span></td><td class="px-4 py-1.5 text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td><td class="px-4 py-1.5 text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td></tr>'),Ad=w('<tr><td colspan="5" class="px-4 py-8 text-center text-(--color-text-muted)">No flagged ASNs</td></tr>'),Nd=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">ASN</th><th class="px-4 py-2">Organization</th><th class="px-4 py-2">Status</th><th class="px-4 py-2 hidden md:table-cell">Events</th><th class="px-4 py-2 hidden md:table-cell">IPs</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),Td=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Firewall</h2> <div class="flex gap-4 border-b border-(--color-border)"><button>IP Rules</button> <button>ASN Reputation</button></div> <!></div>');function Id(e,t){Je(t,!0);let r=W("firewall"),a=W(me({enabled:!1,rules:[]})),o=W(me([])),s=W(!0);async function i(){S(s,!0);const v=await rt("/admin/firewall");v.status===200&&S(a,v.data,!0),S(s,!1)}async function l(){var A;S(s,!0);const v=await rt("/admin/asn-reputation");v.status===200&&S(o,((A=v.data)==null?void 0:A.asn_entries)||[],!0),S(s,!1)}Ke(()=>{n(r)==="firewall"?i():l()});function c(v){return v?new Date(v*1e3).toLocaleString():"Never"}var d=Td(),f=_(u(d),2),g=u(f),b=_(g,2),y=_(f,2);{var p=v=>{var A=yd();m(v,A)},$=v=>{var A=Sd(),C=u(A),E=_(u(C),2),O=u(E),D=_(E,2),T=u(D),z=_(C,2);{var P=B=>{var K=kd(),R=_(u(K));Se(R,21,()=>n(a).rules,$e,(j,V)=>{var G=wd(),le=u(G),M=u(le),N=_(le),Y=u(N),ee=_(N),ce=u(ee);F(ae=>{I(M,n(V).target),I(Y,n(V).reason),I(ce,ae)},[()=>n(V).expires_at===0?"Permanent":c(n(V).expires_at)]),m(j,G)}),m(B,K)},H=B=>{var K=$d();m(B,K)};J(z,B=>{n(a).rules.length>0?B(P):B(H,-1)})}F(()=>{Ie(E,1,`text-[10px] px-1 py-px font-medium ${n(a).enabled?"bg-emerald-500/15 text-emerald-400":"bg-(--color-surface-3) text-(--color-text-muted)"}`),I(O,n(a).enabled?"Enabled":"Disabled"),I(T,`${n(a).rules.length??""} rules`)}),m(v,A)},x=v=>{var A=Nd(),C=u(A),E=u(C),O=_(u(E));Se(O,21,()=>n(o),$e,(D,T)=>{var z=Ed(),P=u(z),H=u(P),B=_(P),K=u(B),R=_(B),j=u(R),V=u(j),G=_(R),le=u(G),M=_(G),N=u(M);F(()=>{I(H,`AS${n(T).asn??""}`),I(K,n(T).as_org),Ie(j,1,`text-[10px] px-1 py-px font-medium
                    ${n(T).status==="blocked"?"bg-red-500/15 text-red-400":n(T).status==="rate_limited"?"bg-orange-500/15 text-orange-400":n(T).status==="watched"?"bg-amber-500/15 text-amber-400":"bg-(--color-surface-3) text-(--color-text-muted)"}`),I(V,n(T).status),I(le,n(T).total_abuse_events),I(N,n(T).abusive_ips)}),m(D,z)},D=>{var T=Ad();m(D,T)}),m(v,A)};J(y,v=>{n(s)?v(p):n(r)==="firewall"?v($,1):v(x,-1)})}F(()=>{Ie(g,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)==="firewall"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),Ie(b,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)==="asn"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`)}),Z("click",g,()=>S(r,"firewall")),Z("click",b,()=>S(r,"asn")),m(e,d),Ge()}Ut(["click"]);var Pd=w('<pre class="text-xs text-emerald-400 bg-emerald-400/10 border border-emerald-400/20 px-3 py-2 whitespace-pre-wrap"> </pre>'),Md=w("<button> </button>"),Od=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Cd=w('<div class="px-4 py-2 bg-(--color-surface-2) text-xs font-semibold uppercase tracking-wider text-(--color-text-secondary) border-b border-(--color-border) sticky top-0 flex items-center justify-between"><span> </span> <button class="p-0.5 text-(--color-text-muted) hover:text-red-400"><!></button></div>'),Ld=w('<div class="px-2 py-1 flex items-center gap-1 text-sm hover:bg-(--color-surface-2)/50 border-b border-(--color-border)/30 group"><span class="text-[10px] text-(--color-text-muted) w-6 text-right tabular-nums shrink-0"></span> <div class="flex flex-col shrink-0 opacity-0 group-hover:opacity-100 transition-opacity"><button class="text-[8px] text-(--color-text-muted) hover:text-(--color-text-primary) leading-none">&blacktriangle;</button> <button class="text-[8px] text-(--color-text-muted) hover:text-(--color-text-primary) leading-none">&blacktriangledown;</button></div> <span class="flex-1 truncate"> </span> <button class="p-0.5 text-(--color-text-muted) hover:text-red-400 opacity-0 group-hover:opacity-100 transition-opacity"><!></button></div>'),Rd=w('<div class="px-4 py-8 text-center text-(--color-text-muted) text-sm">No items</div>'),Dd=w(`<div class="flex gap-px"><input type="text" class="flex-1 px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
               placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)"/> <button class="px-3 py-1.5 text-sm bg-(--color-surface-3) border border-(--color-border) text-(--color-text-secondary) hover:text-(--color-text-primary)"><!></button></div> <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="max-h-[60vh] overflow-y-auto"></div></div>`,1),zd=w(`<div class="space-y-3"><div class="flex items-center justify-between flex-wrap gap-2"><h2 class="text-lg font-semibold">Content</h2> <button class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-accent) text-(--color-surface-0)
             hover:opacity-80 disabled:opacity-30"><!> </button></div> <!> <div class="flex gap-4 border-b border-(--color-border)"></div> <!></div>`);function jd(e,t){Je(t,!0);let r=W("areas"),a=W(me({characters:[],music:[],areas:[]})),o=W(me({characters:[],music:[],areas:[]})),s=W(!0),i=W(!1),l=W(""),c=W("");async function d(){const R=await rt("/admin/content");R.status===200&&(S(a,R.data,!0),S(o,JSON.parse(JSON.stringify(R.data)),!0)),S(s,!1)}Ke(()=>{d()});function f(){return n(r)==="areas"?n(a).areas:n(r)==="characters"?n(a).characters:n(r)==="music"?n(a).music:[]}function g(R){n(r)==="areas"?n(a).areas=R:n(r)==="characters"?n(a).characters=R:n(r)==="music"&&(n(a).music=R)}function b(R){return n(r)==="music"&&!R.includes(".")}function y(){return JSON.stringify(n(a))!==JSON.stringify(n(o))}function p(){if(!n(c).trim())return;const R=[...f(),n(c).trim()];g(R),S(c,"")}function $(R){const j=[...f()];j.splice(R,1),g(j)}function x(R,j){const V=[...f()],G=R+j;G<0||G>=V.length||([V[R],V[G]]=[V[G],V[R]],g(V))}async function v(){var j,V;if(!y()){S(l,"No changes.");return}S(i,!0),S(l,"");const R=await _a("PUT","/admin/content",n(a));S(i,!1),R.status===200?(S(l,((j=R.data)==null?void 0:j.reload_summary)||"Content saved and applied.",!0),S(o,JSON.parse(JSON.stringify(n(a))),!0)):S(l,((V=R.data)==null?void 0:V.reason)||"Failed to save",!0)}var A=zd(),C=u(A),E=_(u(C),2),O=u(E);es(O,{size:13,strokeWidth:1.5});var D=_(O),T=_(C,2);{var z=R=>{var j=Pd(),V=u(j);F(()=>I(V,n(l))),m(R,j)};J(T,R=>{n(l)&&R(z)})}var P=_(T,2);Se(P,20,()=>[["areas","Areas"],["characters","Characters"],["music","Music"]],$e,(R,j)=>{var V=pe(()=>ho(j,2));let G=()=>n(V)[0],le=()=>n(V)[1];var M=Md(),N=u(M);F(()=>{var Y;Ie(M,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)===G()?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),I(N,`${le()??""} (${((Y=n(a)[G()])==null?void 0:Y.length)??0??""})`)}),Z("click",M,()=>{S(r,G(),!0),S(c,"")}),m(R,M)});var H=_(P,2);{var B=R=>{var j=Od();m(R,j)},K=R=>{var j=Dd(),V=ie(j),G=u(V),le=_(G,2),M=u(le);Qn(M,{size:14,strokeWidth:1.5});var N=_(V,2),Y=u(N);Se(Y,21,f,$e,(ee,ce,ae)=>{var k=ue(),L=ie(k);{var q=Te=>{var xe=Cd(),Me=u(xe),fe=u(Me),ve=_(Me,2),be=u(ve);uo(be,{size:11,strokeWidth:1.5}),F(()=>I(fe,n(ce))),Z("click",ve,()=>$(ae)),m(Te,xe)},Q=pe(()=>b(n(ce))),_e=Te=>{var xe=Ld(),Me=u(xe);Me.textContent=ae+1;var fe=_(Me,2),ve=u(fe),be=_(ve,2),oe=_(fe,2),He=u(oe),Ye=_(oe,2),Ot=u(Ye);uo(Ot,{size:12,strokeWidth:1.5}),F(()=>I(He,n(ce))),Z("click",ve,()=>x(ae,-1)),Z("click",be,()=>x(ae,1)),Z("click",Ye,()=>$(ae)),m(Te,xe)};J(L,Te=>{n(Q)?Te(q):Te(_e,-1)})}m(ee,k)},ee=>{var ce=Rd();m(ee,ce)}),F(ee=>Ta(G,"placeholder",`Add ${ee??""}...`),[()=>n(r)==="music"?"track or category":n(r).slice(0,-1)]),Z("keydown",G,ee=>ee.key==="Enter"&&p()),nr(G,()=>n(c),ee=>S(c,ee)),Z("click",le,p),m(R,j)};J(H,R=>{n(s)?R(B):R(K,-1)})}F(R=>{E.disabled=R,I(D,` ${n(i)?"Saving...":"Save & Apply"}`)},[()=>n(i)||!y()]),Z("click",E,v),m(e,A),Ge()}Ut(["click","keydown"]);var Fd=w('<div class="text-center text-gray-500 mt-20"><h2 class="text-2xl font-semibold mb-2">Coming Soon</h2> <p>The <code class="text-gray-400"> </code> page is not yet implemented.</p></div>'),Bd=w('<div class="flex h-screen bg-(--color-surface-0) text-(--color-text-primary)"><!> <main class="flex-1 overflow-auto p-4 md:p-6 lg:p-8"><!></main></div>');function Wd(e,t){Je(t,!0);let r=W(me(window.location.hash||"#/login"));function a(){S(r,window.location.hash||"#/login",!0)}Ke(()=>(window.addEventListener("hashchange",a),()=>window.removeEventListener("hashchange",a))),Ke(()=>(se.loggedIn?ll():Ko(),()=>Ko()));let o=pe(()=>!se.loggedIn&&n(r)!=="#/login"?(window.location.hash="#/login","login"):se.loggedIn&&n(r)==="#/login"?(window.location.hash="#/dashboard","dashboard"):n(r).slice(2)||"login");var s=ue(),i=ie(s);{var l=d=>{Dl(d,{})},c=d=>{var f=Bd(),g=u(f);Ol(g,{get currentPage(){return n(o)}});var b=_(g,2),y=u(b);{var p=P=>{Yl(P,{})},$=P=>{Nc(P,{})},x=P=>{Lc(P,{})},v=P=>{Fc(P,{})},A=P=>{qc(P,{})},C=P=>{rd(P,{})},E=P=>{pd(P,{})},O=P=>{gd(P,{})},D=P=>{Id(P,{})},T=P=>{jd(P,{})},z=P=>{var H=Fd(),B=_(u(H),2),K=_(u(B)),R=u(K);F(()=>I(R,n(o))),m(P,H)};J(y,P=>{n(o)==="dashboard"?P(p):n(o)==="config"?P($,1):n(o)==="sessions"?P(x,2):n(o)==="traffic"?P(v,3):n(o)==="bans"?P(A,4):n(o)==="moderation"?P(C,5):n(o)==="areas"?P(E,6):n(o)==="users"?P(O,7):n(o)==="firewall"?P(D,8):n(o)==="content"?P(T,9):P(z,-1)})}m(d,f)};J(i,d=>{se.loggedIn?d(c,-1):d(l)})}m(e,s),Ge()}Mi(Wd,{target:document.getElementById("app")});
